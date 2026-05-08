#pragma once

#include "lowhunt.h"

typedef struct {
    char value[512];
    char sources[128];
    int source_count;
    int confidence;
} HarvestHostRecord;

typedef struct {
    char email[256];
    char page[512];
    int confidence;
} HarvestEmailRecord;

typedef struct {
    char domain[256];
    char source_scope[128];
    HarvestHostRecord* hosts;
    int host_count;
    HarvestEmailRecord* emails;
    int email_count;
    int filtered_email_count;
    int pages_scanned;
} HarvestSummary;

void harvester_run(const char* domain, const char* source, int limit,
                   const LowHuntConfig* cfg, HarvestSummary* summary_out);
void harvester_summary_free(HarvestSummary* summary);
void harvester_print_sources(void);
