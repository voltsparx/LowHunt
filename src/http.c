#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "http.h"

typedef struct {
    CURL* easy;
    HttpResult* result;
    HttpTask task;
} ActiveTask;

static size_t write_cb(void* contents, size_t size, size_t nmemb, void* userdata) {
    size_t realsize = size * nmemb;
    HttpResponse* mem = (HttpResponse*)userdata;
    char* ptr = realloc(mem->data, mem->size + realsize + 1);

    if (!ptr) return 0;
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = '\0';
    return realsize;
}

void http_global_init(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void http_global_cleanup(void) {
    curl_global_cleanup();
}

static HttpResult* result_new(const char* url) {
    HttpResult* r = calloc(1, sizeof(HttpResult));
    if (!r) return NULL;
    snprintf(r->url, sizeof(r->url), "%s", url ? url : "");
    r->body.data = calloc(1, 1);
    if (!r->body.data) {
        free(r);
        return NULL;
    }
    return r;
}

static CURL* setup_easy(HttpResult* r, int timeout_ms,
                        const char* user_agent, const char* proxy) {
    CURL* curl = curl_easy_init();
    if (!curl) return NULL;

    curl_easy_setopt(curl, CURLOPT_URL, r->url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r->body);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent ? user_agent : DEFAULT_USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)timeout_ms);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, (long)timeout_ms);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

    if (proxy && proxy[0]) {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
    }

    return curl;
}

static void fill_result(CURL* curl, HttpResult* r, CURLcode code) {
    char* effective = NULL;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r->http_code);
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &r->elapsed_ms);
    r->elapsed_ms *= 1000.0;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective);
    if (effective) snprintf(r->effective_url, sizeof(r->effective_url), "%s", effective);
    r->success = (code == CURLE_OK);
}

HttpResult* http_get(const char* url, int timeout_ms,
                     const char* user_agent, const char* proxy) {
    HttpResult* r = result_new(url);
    CURL* curl;
    CURLcode code;

    if (!r) return NULL;
    curl = setup_easy(r, timeout_ms, user_agent, proxy);
    if (!curl) {
        r->success = false;
        return r;
    }

    code = curl_easy_perform(curl);
    fill_result(curl, r, code);
    curl_easy_cleanup(curl);
    return r;
}

void http_result_free(HttpResult* r) {
    if (!r) return;
    free(r->body.data);
    free(r);
}

void http_multi_run(HttpTask* tasks, int count,
                    int timeout_ms, const char* user_agent,
                    const char* proxy, int max_parallel) {
    CURLM* multi;
    ActiveTask* active;
    int next = 0;
    int still_running = 0;

    if (!tasks || count <= 0) return;
    if (max_parallel < 1) max_parallel = 1;
    if (max_parallel > MAX_THREADS) max_parallel = MAX_THREADS;

    active = calloc((size_t)count, sizeof(ActiveTask));
    if (!active) return;

    multi = curl_multi_init();
    if (!multi) {
        free(active);
        return;
    }

    while (next < count || still_running > 0) {
        while (next < count && still_running < max_parallel) {
            HttpResult* r = result_new(tasks[next].url);
            CURL* easy;

            if (!r) {
                next++;
                continue;
            }

            easy = setup_easy(r, timeout_ms, user_agent, proxy);
            if (!easy) {
                http_result_free(r);
                next++;
                continue;
            }

            active[next].easy = easy;
            active[next].result = r;
            active[next].task = tasks[next];
            curl_easy_setopt(easy, CURLOPT_PRIVATE, &active[next]);
            curl_multi_add_handle(multi, easy);
            next++;
            still_running++;
        }

        curl_multi_perform(multi, &still_running);

        for (;;) {
            int msgs_left = 0;
            CURLMsg* msg = curl_multi_info_read(multi, &msgs_left);
            ActiveTask* at = NULL;

            if (!msg) break;
            if (msg->msg != CURLMSG_DONE) continue;

            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &at);
            if (at) {
                fill_result(msg->easy_handle, at->result, msg->data.result);
                if (at->task.callback) at->task.callback(at->result, at->task.userdata);
                http_result_free(at->result);
            }
            curl_multi_remove_handle(multi, msg->easy_handle);
            curl_easy_cleanup(msg->easy_handle);
        }

        if (still_running > 0) {
            int numfds = 0;
            curl_multi_poll(multi, NULL, 0, 1000, &numfds);
        }
    }

    curl_multi_cleanup(multi);
    free(active);
}
