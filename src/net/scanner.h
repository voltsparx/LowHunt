#pragma once

#include "lowhunt.h"

typedef struct Engine Engine;

void scanner_run(const LowHuntConfig* cfg, ResultStore* store);
void scanner_run_with_engine(const LowHuntConfig* cfg, ResultStore* store,
                             Engine* engine);
