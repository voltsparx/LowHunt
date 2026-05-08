#pragma once

#include <stdbool.h>
#include "lowhunt.h"
#include "correlation.h"
#include "harvester.h"

void output_results(const ResultStore* store, const LowHuntConfig* cfg);
void output_investigation(const ResultStore* store, const HarvestSummary* harvest,
                          const CorrelationSummary* correlation, const LowHuntConfig* cfg);
void output_list_sites(const Site* sites, int count);
