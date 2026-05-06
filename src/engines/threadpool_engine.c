#include <stdlib.h>

#include "engine.h"

static void threadpool_run(Engine* self, HttpTask* tasks, int count,
                           const LowHuntConfig* cfg) {
    (void)self;
    http_multi_run(tasks, count, cfg->timeout_ms, DEFAULT_USER_AGENT,
                   cfg->proxy, cfg->thread_count);
}

Engine* threadpool_engine_create(void) {
    Engine* e = calloc(1, sizeof(Engine));
    if (!e) return NULL;
    e->type = ENGINE_THREADPOOL;
    e->name = "threadpool";
    e->run = threadpool_run;
    return e;
}
