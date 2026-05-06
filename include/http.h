#pragma once

#include <stddef.h>
#include "lowhunt.h"

typedef struct {
    char* data;
    size_t size;
} HttpResponse;

typedef struct {
    char url[512];
    long http_code;
    double elapsed_ms;
    HttpResponse body;
    char effective_url[512];
    bool success;
} HttpResult;

void http_global_init(void);
void http_global_cleanup(void);

HttpResult* http_get(const char* url, int timeout_ms,
                     const char* user_agent, const char* proxy);
void http_result_free(HttpResult* r);

typedef void (*HttpCallback)(HttpResult* result, void* userdata);

typedef struct {
    char url[512];
    void* userdata;
    HttpCallback callback;
} HttpTask;

void http_multi_run(HttpTask* tasks, int count,
                    int timeout_ms, const char* user_agent,
                    const char* proxy, int max_parallel);
