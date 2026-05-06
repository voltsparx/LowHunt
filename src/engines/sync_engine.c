#include <stdlib.h>
#include <string.h>

#include "engine.h"

static void sync_run(Engine* self, HttpTask* tasks, int count,
                     const LowHuntConfig* cfg) {
    (void)self;
    for (int i = 0; i < count; i++) {
        HttpResult* result = http_get(tasks[i].url, cfg->timeout_ms,
                                      DEFAULT_USER_AGENT, cfg->proxy);
        if (tasks[i].callback) tasks[i].callback(result, tasks[i].userdata);
        http_result_free(result);
    }
}

Engine* sync_engine_create(void) {
    Engine* e = calloc(1, sizeof(Engine));
    if (!e) return NULL;
    e->type = ENGINE_SYNC;
    e->name = "sync";
    e->run = sync_run;
    return e;
}
