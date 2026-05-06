#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>

#include "engine.h"

static void pause_between_requests(void) {
#ifdef _WIN32
    Sleep(300);
#else
    usleep(300000);
#endif
}

static void stabilizer_run(Engine* self, HttpTask* tasks, int count,
                           const LowHuntConfig* cfg) {
    (void)self;
    for (int i = 0; i < count; i++) {
        HttpResult* result = http_get(tasks[i].url, cfg->timeout_ms,
                                      DEFAULT_USER_AGENT, cfg->proxy);
        if (tasks[i].callback) tasks[i].callback(result, tasks[i].userdata);
        http_result_free(result);
        pause_between_requests();
    }
}

Engine* stabilizer_engine_create(void) {
    Engine* e = calloc(1, sizeof(Engine));
    if (!e) return NULL;
    e->type = ENGINE_STABILIZER;
    e->name = "stabilizer";
    e->run = stabilizer_run;
    return e;
}
