#include "metadata.h"

static const LowHuntMetadata g_metadata = {
    .name = "LowHunt",
    .version = "v1.0",
    .author = "voltsparx",
    .email = "voltsparx@gmail.com",
    .repo_url = "https://github.com/voltsparx/LowHunt",
    .license_name = "MIT",
    .tagline = "Fast public OSINT for authorized username and domain analysis",
    .manual_name = "lowhunt",
    .install_dir_name = "lowhunt"
};

const LowHuntMetadata* lowhunt_metadata(void) {
    return &g_metadata;
}
