#include <stdio.h>
#include <string.h>

#include "colors.h"
#include "engine.h"

void brief_report_process(const ResultStore* store, const LowHuntConfig* cfg) {
    int found = 0;
    int not_found = 0;
    int unknown = 0;
    int errors = 0;

    if (!store || !cfg) return;

    for (int i = 0; i < store->count; i++) {
        switch (store->results[i].status) {
            case RESULT_FOUND: found++; break;
            case RESULT_NOT_FOUND: not_found++; break;
            case RESULT_UNKNOWN: unknown++; break;
            case RESULT_ERROR: errors++; break;
        }
    }

    printf("\n%s[brief]%s Investigator summary\n", CYAN, RESET);
    printf("Checked %d result(s) across %d target(s).\n", store->count, cfg->target_count);

    if (found == 0) {
        printf("No confirmed public profile hits were found in this run.\n");
    } else if (found < 5) {
        printf("A small public footprint was confirmed, with %d positive hit(s).\n", found);
    } else if (found < 15) {
        printf("A moderate public footprint was confirmed, with %d positive hit(s).\n", found);
    } else {
        printf("A broad public footprint was confirmed, with %d positive hit(s).\n", found);
    }

    if (unknown > 0) {
        printf("%sUnknown checks:%s %d result(s) may need a slower engine or manual review.\n",
               YELLOW, RESET, unknown);
    }
    if (errors > 0) {
        printf("%sErrors:%s %d request(s) failed outright and may benefit from a retry.\n",
               RED, RESET, errors);
    }
    if (cfg->very_verbose) {
        printf("Not-found responses: %d\n", not_found);
        printf("Output mode: %s\n", cfg->output_format);
        printf("Engine preference: %s\n", cfg->engine[0] ? cfg->engine : "auto");
    }
}
