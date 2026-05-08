#pragma once

#include "harvester.h"
#include "lowhunt.h"

typedef struct {
    int found_profiles;
    int exact_email_matches;
    int partial_email_matches;
    int correlated_usernames;
    int host_count;
    int email_count;
    int confidence_score;
    char narrative[512];
} CorrelationSummary;

void correlation_analyze(const ResultStore* store, const HarvestSummary* harvest,
                         const LowHuntConfig* cfg, CorrelationSummary* out);
void correlation_print(const CorrelationSummary* summary, const LowHuntConfig* cfg);
