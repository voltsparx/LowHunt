#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

Site* config_load_sites(const char* path, int* count) {
    long size = 0;
    char* text = read_file(path, &size);
    Site* sites;
    int used = 0;

    if (count) *count = 0;
    if (!text || size <= 0) {
        free(text);
        return NULL;
    }

    sites = calloc(MAX_SITES, sizeof(Site));
    if (!sites) {
        free(text);
        return NULL;
    }

    for (char* p = strchr(text, '{'); p && used < MAX_SITES; p = strchr(p + 1, '{')) {
        char* end = object_end(p);
        char saved;
        char error_type[64] = {0};
        char url[512] = {0};

        if (!end) break;
        saved = *(end + 1);
        *(end + 1) = '\0';

        get_string_field(p, "name", sites[used].name, sizeof(sites[used].name));
        get_string_field(p, "url", url, sizeof(url));
        if (url[0] == '\0') get_string_field(p, "url_template", url, sizeof(url));
        snprintf(sites[used].url_template, sizeof(sites[used].url_template), "%s", url);
        get_string_field(p, "errorMsg", sites[used].error_msg, sizeof(sites[used].error_msg));
        get_string_field(p, "errorUrl", sites[used].error_url, sizeof(sites[used].error_url));
        get_string_field(p, "errorType", error_type, sizeof(error_type));
        get_string_field(p, "category", sites[used].category, sizeof(sites[used].category));
        sites[used].error_code = get_int_field(p, "errorCode", 404);
        sites[used].method = parse_method(error_type);
        sites[used].is_nsfw = get_bool_field(p, "isNSFW", false);

        if (sites[used].name[0] && sites[used].url_template[0]) used++;
        *(end + 1) = saved;
        p = end;
    }

    free(text);
    if (count) *count = used;
    return sites;
}
