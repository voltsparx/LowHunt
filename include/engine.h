#pragma once

#include "http.h"
#include "lowhunt.h"

typedef enum {
    ENGINE_PARALLEL = 0,
    ENGINE_THREADPOOL = 1,
    ENGINE_SYNC = 2,
    ENGINE_ASYNC = 3,
    ENGINE_FUSION = 4,
    ENGINE_STABILIZER = 5,
    ENGINE_INTELLIGENCE = 6
} EngineType;

typedef struct Engine Engine;

struct Engine {
    EngineType type;
    const char* name;
    void (*run)(Engine* self, HttpTask* tasks, int count,
                const LowHuntConfig* cfg);
    void (*init)(Engine* self, const LowHuntConfig* cfg);
    void (*destroy)(Engine* self);
    void* priv;
};

Engine* engine_create(EngineType type);
void engine_free(Engine* e);
EngineType engine_from_str(const char* name);
const char* engine_to_str(EngineType type);

Engine* parallel_engine_create(void);
Engine* threadpool_engine_create(void);
Engine* sync_engine_create(void);
Engine* async_engine_create(void);
Engine* fusion_engine_create(void);
Engine* stabilizer_engine_create(void);
Engine* intelligence_engine_create(void);
void intelligence_process(const ResultStore* store, bool no_color);
