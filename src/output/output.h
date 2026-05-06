#pragma once

#include <stdbool.h>
#include "lowhunt.h"

void output_results(const ResultStore* store, const LowHuntConfig* cfg);
void output_list_sites(const Site* sites, int count);
