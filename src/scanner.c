#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifndef _WIN32
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "colors.h"
#include "engine.h"
#include "http.h"
#include "scanner.h"

typedef struct {
    const Site* site;
    const LowHuntConfig* cfg;
    ResultStore* store;
    char username[128];
    char url[512];
} ScanTaskData;

#ifndef _WIN32
typedef struct {
    int task_index;
    ScanResult result;
} PipeScanRecord;

typedef struct {
    int write_fd;
    const ScanTaskData* tasks;
} ChildRunContext;

typedef struct {
    ChildRunContext* ctx;
    int task_index;
} ChildTaskData;
#endif

static bool site_matches_filter(const Site* site, const LowHuntConfig* cfg) {
    if (cfg->site_filter[0] == '\0') return true;
    return strcasecmp(site->name, cfg->site_filter) == 0;
}

static void build_url(const char* tmpl, const char* username, char* out, size_t out_size) {
    const char* token = strstr(tmpl, "{}");
    const char* token2 = strstr(tmpl, "{username}");

    if (token) {
        size_t prefix = (size_t)(token - tmpl);
        snprintf(out, out_size, "%.*s%s%s", (int)prefix, tmpl, username, token + 2);
    } else if (token2) {
        size_t prefix = (size_t)(token2 - tmpl);
        snprintf(out, out_size, "%.*s%s%s", (int)prefix, tmpl, username, token2 + 10);
    } else {
        snprintf(out, out_size, "%s%s", tmpl, username);
    }
}

static ResultStatus classify_result(const Site* site, const HttpResult* result) {
    if (!result || !result->success) return RESULT_ERROR;

    if (site->method == DETECT_STRING_MATCH && site->error_msg[0]) {
        return strstr(result->body.data ? result->body.data : "", site->error_msg)
            ? RESULT_NOT_FOUND
            : RESULT_FOUND;
    }

    if (site->method == DETECT_RESPONSE_URL && site->error_url[0]) {
        return strcmp(result->effective_url, site->error_url) == 0
            ? RESULT_NOT_FOUND
            : RESULT_FOUND;
    }

    if (result->http_code == site->error_code || result->http_code == 404) {
        return RESULT_NOT_FOUND;
    }
    if (result->http_code >= 200 && result->http_code < 400) {
        return RESULT_FOUND;
    }
    if (result->http_code == 403 || result->http_code == 429) {
        return RESULT_UNKNOWN;
    }
    return RESULT_ERROR;
}

static void build_scan_result(const ScanTaskData* data, const HttpResult* http,
                              ScanResult* result) {
    if (!data || !result) return;

    memset(result, 0, sizeof(*result));
    snprintf(result->username, sizeof(result->username), "%s", data->username);
    snprintf(result->url, sizeof(result->url), "%s", data->url);
    snprintf(result->site_name, sizeof(result->site_name), "%s", data->site->name);
    result->http_code = http ? http->http_code : 0;
    result->response_time_ms = http ? http->elapsed_ms : 0.0;
    result->status = classify_result(data->site, http);
}

static void store_result(ResultStore* store, const ScanResult* result) {
    if (!store || !result) return;
    pthread_mutex_lock(&store->lock);
    if (store->count >= store->capacity) {
        int new_capacity = store->capacity == 0 ? 128 : store->capacity * 2;
        ScanResult* next = realloc(store->results, (size_t)new_capacity * sizeof(ScanResult));
        if (next) {
            store->results = next;
            store->capacity = new_capacity;
        }
    }
    if (store->count < store->capacity) {
        store->results[store->count++] = *result;
    }
    pthread_mutex_unlock(&store->lock);
}

static void print_scan_result(const ScanTaskData* data, const ScanResult* result) {
    const char* reset = data->cfg->no_color ? "" : RESET;
    const char* color;
    if (!data || !result) return;
    if (!data->cfg->print_all && result->status != RESULT_FOUND) return;

    color = data->cfg->no_color ? "" :
        (result->status == RESULT_FOUND ? GREEN :
         result->status == RESULT_NOT_FOUND ? DIM :
         result->status == RESULT_UNKNOWN ? YELLOW : RED);

    printf("%s[%s]%s %-16s %-24s %s (HTTP %ld, %.0f ms)\n",
           color,
           result->status == RESULT_FOUND ? "+" :
           result->status == RESULT_NOT_FOUND ? "-" :
           result->status == RESULT_UNKNOWN ? "?" : "!",
           reset,
           result->username,
           result->site_name,
           result->url,
           result->http_code,
           result->response_time_ms);
}

static void handle_scan_result(const ScanTaskData* data, const ScanResult* result) {
    if (!data || !result) return;
    store_result(data->store, result);
    print_scan_result(data, result);
}

static void on_scan_done(HttpResult* http, void* userdata) {
    ScanTaskData* data = (ScanTaskData*)userdata;
    ScanResult result;
    build_scan_result(data, http, &result);
    handle_scan_result(data, &result);
}

static EngineType choose_engine_type(const LowHuntConfig* cfg, int total_tasks) {
    if (!cfg) return ENGINE_THREADPOOL;
    if (cfg->engine[0] && strcasecmp(cfg->engine, "auto") != 0) {
        return engine_from_str(cfg->engine);
    }

    if (cfg->tor_enabled || cfg->proxy[0]) return ENGINE_STABILIZER;
    if (total_tasks <= 4) return ENGINE_SYNC;
    if (total_tasks <= cfg->thread_count) return ENGINE_PARALLEL;
    if (total_tasks >= 600 && cfg->thread_count >= 16) return ENGINE_FUSION;
    if (total_tasks >= 200) return ENGINE_ASYNC;
    return ENGINE_THREADPOOL;
}

static EngineType fallback_engine_type(EngineType primary) {
    switch (primary) {
        case ENGINE_SYNC:
        case ENGINE_STABILIZER:
            return ENGINE_THREADPOOL;
        case ENGINE_PARALLEL:
        case ENGINE_THREADPOOL:
        case ENGINE_ASYNC:
        case ENGINE_FUSION:
        case ENGINE_INTELLIGENCE:
        default:
            return ENGINE_SYNC;
    }
}

static int count_pending_tasks(const bool* completed, int total) {
    int pending = 0;
    for (int i = 0; i < total; i++) {
        if (!completed[i]) pending++;
    }
    return pending;
}

static void run_pending_tasks(Engine* engine, HttpTask* tasks, const bool* completed,
                              int total, const LowHuntConfig* cfg) {
    HttpTask* subset;
    int index = 0;
    int pending = count_pending_tasks(completed, total);

    if (!engine || !engine->run || !tasks || pending <= 0) return;

    subset = calloc((size_t)pending, sizeof(HttpTask));
    if (!subset) return;

    for (int i = 0; i < total; i++) {
        if (completed[i]) continue;
        subset[index++] = tasks[i];
    }

    engine->run(engine, subset, pending, cfg);
    free(subset);
}

#ifndef _WIN32
static bool write_all(int fd, const void* buf, size_t len) {
    const char* ptr = (const char*)buf;
    while (len > 0) {
        ssize_t written = write(fd, ptr, len);
        if (written < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        ptr += written;
        len -= (size_t)written;
    }
    return true;
}

static void child_scan_done(HttpResult* http, void* userdata) {
    ChildTaskData* child = (ChildTaskData*)userdata;
    PipeScanRecord record;

    if (!child || !child->ctx || !child->ctx->tasks) return;
    memset(&record, 0, sizeof(record));
    record.task_index = child->task_index;
    build_scan_result(&child->ctx->tasks[child->task_index], http, &record.result);
    (void)write_all(child->ctx->write_fd, &record, sizeof(record));
}

static bool run_engine_isolated(Engine* engine, HttpTask* tasks, ScanTaskData* data,
                                int total, const LowHuntConfig* cfg, bool* completed) {
    int pipefd[2];
    pid_t pid;
    ChildRunContext child_ctx;
    ChildTaskData* child_data;
    HttpTask* child_tasks;
    bool clean_exit = false;
    bool all_results = false;
    int completed_count = 0;

    if (!engine || !engine->run || !tasks || !data || total <= 0 || !completed) return false;
    if (pipe(pipefd) != 0) return false;

    pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (pid == 0) {
        close(pipefd[0]);
        child_ctx.write_fd = pipefd[1];
        child_ctx.tasks = data;
        child_data = calloc((size_t)total, sizeof(ChildTaskData));
        child_tasks = calloc((size_t)total, sizeof(HttpTask));
        if (!child_data || !child_tasks) _exit(2);

        for (int i = 0; i < total; i++) {
            child_data[i].ctx = &child_ctx;
            child_data[i].task_index = i;
            child_tasks[i] = tasks[i];
            child_tasks[i].userdata = &child_data[i];
            child_tasks[i].callback = child_scan_done;
        }

        if (engine->init) engine->init(engine, cfg);
        engine->run(engine, child_tasks, total, cfg);
        free(child_tasks);
        free(child_data);
        close(pipefd[1]);
        _exit(0);
    }

    close(pipefd[1]);
    for (;;) {
        PipeScanRecord record;
        size_t got = 0;
        char* out = (char*)&record;

        while (got < sizeof(record)) {
            ssize_t n = read(pipefd[0], out + got, sizeof(record) - got);
            if (n == 0) goto read_done;
            if (n < 0) {
                if (errno == EINTR) continue;
                goto read_done;
            }
            got += (size_t)n;
        }

        if (record.task_index < 0 || record.task_index >= total) continue;
        if (completed[record.task_index]) continue;
        completed[record.task_index] = true;
        completed_count++;
        handle_scan_result(&data[record.task_index], &record.result);
    }

read_done:
    close(pipefd[0]);
    {
        int status = 0;
        if (waitpid(pid, &status, 0) == pid) {
            clean_exit = WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }
    }
    all_results = (completed_count == total);
    return clean_exit && all_results;
}
#endif

static int build_scan_tasks(const LowHuntConfig* cfg, ResultStore* store,
                            HttpTask** tasks_out, ScanTaskData** data_out) {
    HttpTask* tasks;
    ScanTaskData* data;
    int total = 0;
    int idx = 0;

    if (!cfg || !store || !tasks_out || !data_out) return 0;

    for (int t = 0; t < cfg->target_count; t++) {
        for (int s = 0; s < cfg->site_count; s++) {
            if (!cfg->nsfw && cfg->sites[s].is_nsfw) continue;
            if (!site_matches_filter(&cfg->sites[s], cfg)) continue;
            total++;
        }
    }

    if (total <= 0) return 0;

    tasks = calloc((size_t)total, sizeof(HttpTask));
    data = calloc((size_t)total, sizeof(ScanTaskData));
    if (!tasks || !data) {
        free(tasks);
        free(data);
        return 0;
    }

    for (int t = 0; t < cfg->target_count; t++) {
        for (int s = 0; s < cfg->site_count; s++) {
            if (!cfg->nsfw && cfg->sites[s].is_nsfw) continue;
            if (!site_matches_filter(&cfg->sites[s], cfg)) continue;

            data[idx].site = &cfg->sites[s];
            data[idx].cfg = cfg;
            data[idx].store = store;
            snprintf(data[idx].username, sizeof(data[idx].username), "%s", cfg->targets[t]);
            build_url(cfg->sites[s].url_template, cfg->targets[t], data[idx].url, sizeof(data[idx].url));

            snprintf(tasks[idx].url, sizeof(tasks[idx].url), "%s", data[idx].url);
            tasks[idx].userdata = &data[idx];
            tasks[idx].callback = on_scan_done;
            idx++;
        }
    }

    *tasks_out = tasks;
    *data_out = data;
    return idx;
}

void scanner_run(const LowHuntConfig* cfg, ResultStore* store) {
    Engine* engine;
    EngineType type;

    if (!cfg || !store) return;
    type = choose_engine_type(cfg, cfg->target_count * cfg->site_count);
    engine = engine_create(type);
    scanner_run_with_engine(cfg, store, engine);
    engine_free(engine);
}

void scanner_run_with_engine(const LowHuntConfig* cfg, ResultStore* store,
                             Engine* engine) {
    HttpTask* tasks = NULL;
    ScanTaskData* data = NULL;
    bool* completed = NULL;
    bool own_engine = false;
    EngineType selected_type;
    int total;

    if (!cfg || !store || !cfg->sites || cfg->site_count <= 0 || cfg->target_count <= 0) return;

    total = build_scan_tasks(cfg, store, &tasks, &data);
    if (total <= 0) {
        free(tasks);
        free(data);
        return;
    }

    selected_type = engine ? engine->type : choose_engine_type(cfg, total);
    if (!engine) {
        engine = engine_create(selected_type);
        own_engine = true;
    }
    if (!engine || !engine->run) {
        fprintf(stderr, "%s[WARN]%s Engine unavailable, falling back to threadpool.\n",
                cfg->no_color ? "" : YELLOW, cfg->no_color ? "" : RESET);
        if (own_engine) engine_free(engine);
        engine = engine_create(ENGINE_THREADPOOL);
        own_engine = true;
        selected_type = ENGINE_THREADPOOL;
    }

    completed = calloc((size_t)total, sizeof(bool));
    if (!completed) {
        free(tasks);
        free(data);
        if (own_engine && engine) engine_free(engine);
        return;
    }

    printf("Scanning %d target/site combinations with engine '%s'...\n\n",
           total, engine ? engine->name : "threadpool");

#ifndef _WIN32
    Engine* fallback = NULL;
    EngineType fallback_type;

    if (engine && run_engine_isolated(engine, tasks, data, total, cfg, completed)) {
        free(completed);
        free(tasks);
        free(data);
        if (own_engine && engine) engine_free(engine);
        return;
    }

    if (count_pending_tasks(completed, total) > 0) {
        fallback_type = fallback_engine_type(selected_type);
        fallback = engine_create(fallback_type);
        if (fallback && fallback->run) {
            fprintf(stderr,
                    "%s[WARN]%s Engine '%s' did not finish cleanly. Recovering remaining tasks with '%s'.\n",
                    cfg->no_color ? "" : YELLOW, cfg->no_color ? "" : RESET,
                    engine ? engine->name : engine_to_str(selected_type),
                    fallback->name);
            run_pending_tasks(fallback, tasks, completed, total, cfg);
        } else {
            fprintf(stderr,
                    "%s[WARN]%s Could not create fallback engine. Remaining tasks were skipped.\n",
                    cfg->no_color ? "" : YELLOW, cfg->no_color ? "" : RESET);
        }
        engine_free(fallback);
    }
#else
    if (engine && engine->run) {
        if (engine->init) engine->init(engine, cfg);
        engine->run(engine, tasks, total, cfg);
    }
#endif

    free(completed);
    free(tasks);
    free(data);
    if (own_engine && engine) engine_free(engine);
}
