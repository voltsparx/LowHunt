#pragma once

#include "lowhunt.h"

typedef void (*HarvestHostSink)(const char* host, const char* source_name, void* userdata);

int harvest_source_collect(const char* source_name, const char* domain,
                           const LowHuntConfig* cfg, HarvestHostSink sink,
                           void* userdata);
void harvest_source_describe_all(void);
