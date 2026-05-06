#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "colors.h"
#include "harvester.h"
#include "http.h"

static bool contains_domain(const char* host, const char* domain) {
    size_t hlen = strlen(host);
    size_t dlen = strlen(domain);
    if (hlen < dlen) return false;
    if (strcasecmp(host, domain) == 0) return true;
    return hlen > dlen && host[hlen - dlen - 1] == '.' &&
           strcasecmp(host + hlen - dlen, domain) == 0;
}

static void add_unique(char*** items, int* count, int* capacity, const char* value) {
    for (int i = 0; i < *count; i++) {
        if (strcasecmp((*items)[i], value) == 0) return;
    }
    if (*count >= *capacity) {
        int next_capacity = *capacity == 0 ? 64 : *capacity * 2;
        char** next = realloc(*items, (size_t)next_capacity * sizeof(char*));
        if (!next) return;
        *items = next;
        *capacity = next_capacity;
    }
    (*items)[*count] = malloc(strlen(value) + 1);
    if ((*items)[*count]) {
        strcpy((*items)[*count], value);
        (*count)++;
    }
}

static void extract_crtsh_hosts(const char* body, const char* domain,
                                char*** items, int* count, int* capacity) {
    const char* key = "\"name_value\"";
    const char* p = body;

    while ((p = strstr(p, key)) != NULL) {
        const char* colon = strchr(p, ':');
        const char* first;
        const char* second;
        char host[512];
        size_t len;

        if (!colon) break;
        first = strchr(colon, '"');
        if (!first) break;
        second = strchr(first + 1, '"');
        if (!second) break;
        len = (size_t)(second - first - 1);
        if (len >= sizeof(host)) len = sizeof(host) - 1;
        memcpy(host, first + 1, len);
        host[len] = '\0';

        for (char* line = strtok(host, "\\n"); line; line = strtok(NULL, "\\n")) {
            while (*line == '*' || *line == '.') line++;
            if (contains_domain(line, domain)) add_unique(items, count, capacity, line);
        }
        p = second + 1;
    }
}

static void write_harvest_output(char** hosts, int count, const char* domain,
                                 const LowHuntConfig* cfg) {
    FILE* f;

    if (!cfg || cfg->output_file[0] == '\0') return;
    f = fopen(cfg->output_file, "w");
    if (!f) {
        fprintf(stderr, "%s[ERROR]%s Cannot open output file: %s\n",
                cfg->no_color ? "" : RED, cfg->no_color ? "" : RESET,
                cfg->output_file);
        return;
    }

    if (strcmp(cfg->output_format, "json") == 0) {
        fprintf(f, "{\n  \"domain\": \"%s\",\n  \"hosts\": [\n", domain);
        for (int i = 0; i < count; i++) {
            fprintf(f, "    \"%s\"%s\n", hosts[i], i + 1 < count ? "," : "");
        }
        fprintf(f, "  ]\n}\n");
    } else if (strcmp(cfg->output_format, "csv") == 0) {
        fprintf(f, "domain,host\n");
        for (int i = 0; i < count; i++) fprintf(f, "%s,%s\n", domain, hosts[i]);
    } else {
        fprintf(f, "LowHunt host harvest for %s\n", domain);
        fprintf(f, "========================================\n");
        for (int i = 0; i < count; i++) fprintf(f, "%s\n", hosts[i]);
    }
    fclose(f);
    printf("\n%s[+]%s Harvest written to: %s\n",
           cfg->no_color ? "" : GREEN, cfg->no_color ? "" : RESET,
           cfg->output_file);
}

void harvester_run(const char* domain, const char* source, int limit,
                   const LowHuntConfig* cfg) {
    char url[512];
    HttpResult* response;
    char** hosts = NULL;
    int count = 0;
    int capacity = 0;
    const char* selected = (source && source[0]) ? source : "crtsh";

    if (!domain || !domain[0]) return;
    if (strcasecmp(selected, "crtsh") != 0 && strcasecmp(selected, "all") != 0) {
        printf("%s[!]%s Source '%s' is not implemented yet; using crtsh.\n",
               cfg->no_color ? "" : YELLOW, cfg->no_color ? "" : RESET, selected);
    }

    snprintf(url, sizeof(url),
             "https://crt.sh/?q=%%25.%s&exclude=expired&deduplicate=Y&output=json",
             domain);

    printf("Harvesting public certificate hostnames for %s via crt.sh...\n\n", domain);
    response = http_get(url, cfg->timeout_ms, DEFAULT_USER_AGENT, cfg->proxy);
    if (!response || !response->success) {
        printf("%s[!]%s crt.sh request failed.\n",
               cfg->no_color ? "" : RED, cfg->no_color ? "" : RESET);
        http_result_free(response);
        return;
    }

    extract_crtsh_hosts(response->body.data, domain, &hosts, &count, &capacity);
    int display_count = count;
    if (limit > 0 && display_count > limit) display_count = limit;

    for (int i = 0; i < display_count; i++) {
        printf("%s[+]%s %s\n", cfg->no_color ? "" : GREEN, cfg->no_color ? "" : RESET, hosts[i]);
    }
    printf("\nTotal hosts: %d\n", display_count);
    write_harvest_output(hosts, display_count, domain, cfg);

    (void)capacity;
    for (int i = 0; i < count; i++) free(hosts ? hosts[i] : NULL);
    free(hosts);
    http_result_free(response);
}
