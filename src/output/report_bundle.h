#pragma once

#include "harvester.h"
#include "lowhunt.h"

void report_bundle_write_scan(const ResultStore* store, const LowHuntConfig* cfg);
void report_bundle_write_harvest(const HarvestSummary* summary, const LowHuntConfig* cfg);
