#include <stdio.h>
#include <string.h>
#include <time.h>

#include "colors.h"
#include "output.h"

static const char* status_str(ResultStatus s) {
    switch (s) {
        case RESULT_FOUND: return "FOUND";
        case RESULT_NOT_FOUND: return "NOT_FOUND";
        case RESULT_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void output_list_sites(const Site* sites, int count, bool no_color) {
    const char* bold = no_color ? "" : BOLD;
    const char* reset = no_color ? "" : RESET;

    printf("%s%-4s %-26s %-16s %s%s\n", bold, "#", "Name", "Category", "NSFW", reset);
    printf("--------------------------------------------------------\n");
    for (int i = 0; i < count; i++) {
        printf("%-4d %-26s %-16s %s\n",
               i + 1,
               sites[i].name,
               sites[i].category[0] ? sites[i].category : "general",
               sites[i].is_nsfw ? "yes" : "no");
    }
    printf("\nTotal: %d platforms\n", count);
}

void output_results(const ResultStore* store, const LowHuntConfig* cfg) {
    FILE* f;
    time_t now;
    char timebuf[64];
    int emitted = 0;

    if (!store || !cfg || cfg->output_file[0] == '\0') return;

    f = fopen(cfg->output_file, "w");
    if (!f) {
        fprintf(stderr, "%s[ERROR]%s Cannot open output file: %s\n",
                cfg->no_color ? "" : RED, cfg->no_color ? "" : RESET,
                cfg->output_file);
        return;
    }

    now = time(NULL);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%S%z", localtime(&now));

    if (strcmp(cfg->output_format, "json") == 0) {
        fprintf(f, "{\n  \"tool\": \"LowHunt\",\n  \"version\": \"%s\",\n", LOWHUNT_VERSION);
        fprintf(f, "  \"generated\": \"%s\",\n  \"results\": [\n", timebuf);
        for (int i = 0; i < store->count; i++) {
            const ScanResult* r = &store->results[i];
            if (!cfg->print_all && r->status != RESULT_FOUND) continue;
            if (emitted++ > 0) fprintf(f, ",\n");
            fprintf(f,
                    "    {\"username\":\"%s\",\"site\":\"%s\",\"status\":\"%s\","
                    "\"url\":\"%s\",\"http_code\":%ld,\"response_time_ms\":%.2f}",
                    r->username, r->site_name, status_str(r->status),
                    r->url, r->http_code, r->response_time_ms);
        }
        fprintf(f, "\n  ]\n}\n");
    } else if (strcmp(cfg->output_format, "csv") == 0) {
        fprintf(f, "username,site,status,url,http_code,response_time_ms\n");
        for (int i = 0; i < store->count; i++) {
            const ScanResult* r = &store->results[i];
            if (!cfg->print_all && r->status != RESULT_FOUND) continue;
            fprintf(f, "\"%s\",\"%s\",\"%s\",\"%s\",%ld,%.2f\n",
                    r->username, r->site_name, status_str(r->status),
                    r->url, r->http_code, r->response_time_ms);
        }
    } else {
        fprintf(f, "LowHunt v%s | %s\n", LOWHUNT_VERSION, timebuf);
        fprintf(f, "========================================\n\n");
        for (int i = 0; i < store->count; i++) {
            const ScanResult* r = &store->results[i];
            if (!cfg->print_all && r->status != RESULT_FOUND) continue;
            fprintf(f, "[%s] %-24s %s\n", status_str(r->status), r->site_name, r->url);
        }
    }

    fclose(f);
    printf("\n%s[+]%s Results written to: %s\n",
           cfg->no_color ? "" : GREEN, cfg->no_color ? "" : RESET,
           cfg->output_file);
}
