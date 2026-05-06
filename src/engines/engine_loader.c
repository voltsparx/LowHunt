#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "engine.h"

EngineType engine_from_str(const char* name) {
    if (!name || !name[0]) return ENGINE_THREADPOOL;
    if (strcasecmp(name, "parallel") == 0) return ENGINE_PARALLEL;
    if (strcasecmp(name, "threadpool") == 0) return ENGINE_THREADPOOL;
    if (strcasecmp(name, "sync") == 0) return ENGINE_SYNC;
    if (strcasecmp(name, "async") == 0) return ENGINE_ASYNC;
    if (strcasecmp(name, "fusion") == 0) return ENGINE_FUSION;
    if (strcasecmp(name, "stabilizer") == 0) return ENGINE_STABILIZER;
    if (strcasecmp(name, "intelligence") == 0) return ENGINE_INTELLIGENCE;
    return ENGINE_THREADPOOL;
}

const char* engine_to_str(EngineType type) {
    switch (type) {
        case ENGINE_PARALLEL: return "parallel";
        case ENGINE_THREADPOOL: return "threadpool";
        case ENGINE_SYNC: return "sync";
        case ENGINE_ASYNC: return "async";
        case ENGINE_FUSION: return "fusion";
        case ENGINE_STABILIZER: return "stabilizer";
        case ENGINE_INTELLIGENCE: return "intelligence";
        default: return "threadpool";
    }
}

Engine* engine_create(EngineType type) {
    switch (type) {
        case ENGINE_PARALLEL: return parallel_engine_create();
        case ENGINE_SYNC: return sync_engine_create();
        case ENGINE_ASYNC: return async_engine_create();
        case ENGINE_FUSION: return fusion_engine_create();
        case ENGINE_STABILIZER: return stabilizer_engine_create();
        case ENGINE_INTELLIGENCE: return intelligence_engine_create();
        case ENGINE_THREADPOOL:
        default: return threadpool_engine_create();
    }
}

void engine_free(Engine* e) {
    if (!e) return;
    if (e->destroy) e->destroy(e);
    free(e);
}
