#pragma once

typedef struct {
    const char* name;
    const char* version;
    const char* author;
    const char* email;
    const char* repo_url;
    const char* license_name;
    const char* tagline;
    const char* manual_name;
    const char* install_dir_name;
} LowHuntMetadata;

const LowHuntMetadata* lowhunt_metadata(void);
