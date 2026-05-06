#pragma once

#include "lowhunt.h"

void harvester_run(const char* domain, const char* source, int limit,
                   const LowHuntConfig* cfg);
