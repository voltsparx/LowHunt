#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "colors.h"
#include "http.h"
#include "scanner.h"

typedef struct {
    const Site* site;
    const LowHuntConfig* cfg;
    ResultStore* store;
    char username[128];
    char url[512];
} ScanTaskData;

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

static void on_scan_done(HttpResult* http, void* userdata) {
    ScanTaskData* data = (ScanTaskData*)userdata;
    ScanResult result;
    const char* reset = data->cfg->no_color ? "" : RESET;
    const char* color;

    memset(&result, 0, sizeof(result));
    snprintf(result.username, sizeof(result.username), "%s", data->username);
    snprintf(result.url, sizeof(result.url), "%s", data->url);
    snprintf(result.site_name, sizeof(result.site_name), "%s", data->site->name);
    result.http_code = http ? http->http_code : 0;
    result.response_time_ms = http ? http->elapsed_ms : 0.0;
    result.status = classify_result(data->site, http);

    store_result(data->store, &result);

    if (!data->cfg->print_all && result.status != RESULT_FOUND) return;

    color = data->cfg->no_color ? "" :
        (result.status == RESULT_FOUND ? GREEN :
         result.status == RESULT_NOT_FOUND ? DIM :
         result.status == RESULT_UNKNOWN ? YELLOW : RED);

    printf("%s[%s]%s %-16s %-24s %s (HTTP %ld, %.0f ms)\n",
           color,
           result.status == RESULT_FOUND ? "+" :
           result.status == RESULT_NOT_FOUND ? "-" :
           result.status == RESULT_UNKNOWN ? "?" : "!",
           reset,
           result.username,
           result.site_name,
           result.url,
           result.http_code,
           result.response_time_ms);
}

void scanner_run(const LowHuntConfig* cfg, ResultStore* store) {
    HttpTask* tasks;
    ScanTaskData* data;
    int total = 0;
    int idx = 0;

    if (!cfg || !store || !cfg->sites || cfg->site_count <= 0 || cfg->target_count <= 0) return;

    for (int t = 0; t < cfg->target_count; t++) {
        for (int s = 0; s < cfg->site_count; s++) {
            if (!cfg->nsfw && cfg->sites[s].is_nsfw) continue;
            if (!site_matches_filter(&cfg->sites[s], cfg)) continue;
            total++;
        }
    }

    tasks = calloc((size_t)total, sizeof(HttpTask));
    data = calloc((size_t)total, sizeof(ScanTaskData));
    if (!tasks || !data) {
        free(tasks);
        free(data);
        return;
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

    printf("Scanning %d target/site combinations with %d workers...\n\n", idx, cfg->thread_count);
    http_multi_run(tasks, idx, cfg->timeout_ms, DEFAULT_USER_AGENT, cfg->proxy, cfg->thread_count);

    free(tasks);
    free(data);
}
