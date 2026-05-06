#include <stdlib.h>

#include "engine.h"

static void parallel_run(Engine* self, HttpTask* tasks, int count,
                         const LowHuntConfig* cfg) {
    (void)self;
    int cap = count > 0 ? count : 1;
    http_multi_run(tasks, count, cfg->timeout_ms, DEFAULT_USER_AGENT,
                   cfg->proxy, cap);
}

Engine* parallel_engine_create(void) {
    Engine* e = calloc(1, sizeof(Engine));
    if (!e) return NULL;
    e->type = ENGINE_PARALLEL;
    e->name = "parallel";
    e->run = parallel_run;
    return e;
}
