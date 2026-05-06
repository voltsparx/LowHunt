#include <stdlib.h>

#include "engine.h"

static void async_run(Engine* self, HttpTask* tasks, int count,
                      const LowHuntConfig* cfg) {
    (void)self;
    int cap = cfg->thread_count > 0 ? cfg->thread_count : 20;
    if (cap < 1) cap = 1;
    http_multi_run(tasks, count, cfg->timeout_ms, DEFAULT_USER_AGENT,
                   cfg->proxy, cap);
}

Engine* async_engine_create(void) {
    Engine* e = calloc(1, sizeof(Engine));
    if (!e) return NULL;
    e->type = ENGINE_ASYNC;
    e->name = "async";
    e->run = async_run;
    return e;
}
