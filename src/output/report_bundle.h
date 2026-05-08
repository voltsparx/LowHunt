#pragma once

#include "harvester.h"
#include "lowhunt.h"
#include "correlation.h"

void report_bundle_write_scan(const ResultStore* store, const LowHuntConfig* cfg);
void report_bundle_write_harvest(const HarvestSummary* summary, const LowHuntConfig* cfg);
void report_bundle_write_investigation(const ResultStore* store, const HarvestSummary* summary,
                                       const CorrelationSummary* correlation,
                                       const LowHuntConfig* cfg);
