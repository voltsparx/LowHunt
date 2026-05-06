#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"

static char* read_file(const char* path, long* size_out) {
    FILE* f = fopen(path, "rb");
    char* buf;
    long size;

    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    rewind(f);

    buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[size] = '\0';
    fclose(f);
    if (size_out) *size_out = size;
    return buf;
}

static char* object_end(char* start) {
    int depth = 0;
    bool in_string = false;
    bool escape = false;

    for (char* p = start; *p; p++) {
        if (escape) {
            escape = false;
            continue;
        }
        if (*p == '\\' && in_string) {
            escape = true;
            continue;
        }
        if (*p == '"') in_string = !in_string;
        if (in_string) continue;
        if (*p == '{') depth++;
        if (*p == '}') {
            depth--;
            if (depth == 0) return p;
        }
    }
    return NULL;
}

static void get_string_field(const char* obj, const char* key,
                             char* out, size_t out_size) {
    char pattern[64];
    const char* p;
    const char* q;
    size_t i = 0;

    if (!out || out_size == 0) return;
    out[0] = '\0';
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(obj, pattern);
    if (!p) return;
    p = strchr(p + strlen(pattern), ':');
    if (!p) return;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '"') return;
    p++;
    q = p;
    while (*q && (*q != '"' || (*(q - 1) == '\\'))) {
        if (i + 1 < out_size) out[i++] = *q;
        q++;
    }
    out[i] = '\0';
}

static int get_int_field(const char* obj, const char* key, int fallback) {
    char pattern[64];
    const char* p;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(obj, pattern);
    if (!p) return fallback;
    p = strchr(p + strlen(pattern), ':');
    if (!p) return fallback;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    return atoi(p);
}

static bool get_bool_field(const char* obj, const char* key, bool fallback) {
    char pattern[64];
    const char* p;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(obj, pattern);
    if (!p) return fallback;
    p = strchr(p + strlen(pattern), ':');
    if (!p) return fallback;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (strncmp(p, "true", 4) == 0) return true;
    if (strncmp(p, "false", 5) == 0) return false;
    return fallback;
}

static DetectMethod parse_method(const char* value) {
    if (strcmp(value, "message") == 0 || strcmp(value, "string") == 0) {
        return DETECT_STRING_MATCH;
    }
    if (strcmp(value, "response_url") == 0 || strcmp(value, "url") == 0) {
        return DETECT_RESPONSE_URL;
    }
    return DETECT_STATUS_CODE;
}

static bool parse_site_object(char* object, Site* site) {
    char error_type[64] = {0};
    char url[512] = {0};

    memset(site, 0, sizeof(*site));
    get_string_field(object, "name", site->name, sizeof(site->name));
    get_string_field(object, "url", url, sizeof(url));
    if (url[0] == '\0') get_string_field(object, "url_template", url, sizeof(url));
    snprintf(site->url_template, sizeof(site->url_template), "%s", url);
    get_string_field(object, "errorMsg", site->error_msg, sizeof(site->error_msg));
    get_string_field(object, "errorUrl", site->error_url, sizeof(site->error_url));
    get_string_field(object, "errorType", error_type, sizeof(error_type));
    get_string_field(object, "category", site->category, sizeof(site->category));
    site->error_code = get_int_field(object, "errorCode", 404);
    site->method = parse_method(error_type);
    site->is_nsfw = get_bool_field(object, "isNSFW", false);

    return site->name[0] != '\0' && site->url_template[0] != '\0';
}

static int parse_sites_from_text(char* text, Site* sites, int used) {
    for (char* p = strchr(text, '{'); p && used < MAX_SITES; p = strchr(p + 1, '{')) {
        char* end = object_end(p);
        char saved;
        Site site;

        if (!end) break;
        saved = *(end + 1);
        *(end + 1) = '\0';

        if (parse_site_object(p, &site)) sites[used++] = site;

        *(end + 1) = saved;
        p = end;
    }

    return used;
}

static bool has_json_suffix(const char* name) {
    size_t len = strlen(name);
    return len > 5 && strcmp(name + len - 5, ".json") == 0;
}

static int load_sites_from_file(const char* path, Site* sites, int used) {
    long size = 0;
    char* text = read_file(path, &size);
    if (!text || size <= 0) {
        free(text);
        return used;
    }

    used = parse_sites_from_text(text, sites, used);
    free(text);
    return used;
}

static int load_sites_from_directory(const char* path, Site* sites, int used) {
    DIR* dir = opendir(path);
    struct dirent* ent;

    if (!dir) return used;
    while ((ent = readdir(dir)) != NULL && used < MAX_SITES) {
        char full_path[1024];
        if (!has_json_suffix(ent->d_name)) continue;
        snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);
        used = load_sites_from_file(full_path, sites, used);
    }
    closedir(dir);
    return used;
}

static int compare_sites_by_name(const void* a, const void* b) {
    const Site* left = (const Site*)a;
    const Site* right = (const Site*)b;
    return strcmp(left->name, right->name);
}

Site* config_load_sites(const char* path, int* count) {
    struct stat st;
    Site* sites;
    int used = 0;

    if (count) *count = 0;
    sites = calloc(MAX_SITES, sizeof(Site));
    if (!sites) return NULL;

    if (stat(path, &st) != 0) {
        free(sites);
        return NULL;
    }

    if (S_ISDIR(st.st_mode)) {
        used = load_sites_from_directory(path, sites, used);
    } else {
        used = load_sites_from_file(path, sites, used);
    }

    if (used > 1) qsort(sites, (size_t)used, sizeof(Site), compare_sites_by_name);
    if (count) *count = used;
    return sites;
}
