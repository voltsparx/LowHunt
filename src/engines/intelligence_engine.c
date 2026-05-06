#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "engine.h"

static void intelligence_run(Engine* self, HttpTask* tasks, int count,
                             const LowHuntConfig* cfg) {
    (void)self;
    http_multi_run(tasks, count, cfg->timeout_ms, DEFAULT_USER_AGENT,
                   cfg->proxy, cfg->thread_count);
}

void intelligence_process(const ResultStore* store) {
    int found = 0;
    int unknown = 0;
    int errors = 0;

    if (!store) return;
    for (int i = 0; i < store->count; i++) {
        if (store->results[i].status == RESULT_FOUND) found++;
        else if (store->results[i].status == RESULT_UNKNOWN) unknown++;
        else if (store->results[i].status == RESULT_ERROR) errors++;
    }

    printf("\n%s[intel]%s Findings summary\n", CYAN, RESET);
    printf("  Found profiles : %d\n", found);
    printf("  Unknown checks : %d\n", unknown);
    printf("  Errors         : %d\n", errors);
    if (found >= 10) {
        printf("%s  Signal         : broad public footprint%s\n", YELLOW, RESET);
    } else if (found > 0) {
        printf("  Signal         : limited public footprint\n");
    } else {
        printf("  Signal         : no confirmed public profiles in this run\n");
    }
}

Engine* intelligence_engine_create(void) {
    Engine* e = calloc(1, sizeof(Engine));
    if (!e) return NULL;
    e->type = ENGINE_INTELLIGENCE;
    e->name = "intelligence";
    e->run = intelligence_run;
    return e;
}
