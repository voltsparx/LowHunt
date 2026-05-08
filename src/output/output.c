#include <stdio.h>
#include <string.h>
#include <time.h>

#include "colors.h"
#include "metadata.h"
#include "output.h"

static const char* status_str(ResultStatus s) {
    switch (s) {
        case RESULT_FOUND: return "FOUND";
        case RESULT_NOT_FOUND: return "NOT_FOUND";
        case RESULT_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

static void fprint_json_string(FILE* f, const char* s) {
    if (!f) return;
    fputc('"', f);
    for (const char* p = s ? s : ""; *p; p++) {
        switch (*p) {
            case '\\': fputs("\\\\", f); break;
            case '"': fputs("\\\"", f); break;
            case '\n': fputs("\\n", f); break;
            case '\r': fputs("\\r", f); break;
            case '\t': fputs("\\t", f); break;
            default: fputc(*p, f); break;
        }
    }
    fputc('"', f);
}

static void summarize_counts(const ResultStore* store, int* found, int* not_found,
                             int* unknown, int* errors) {
    if (found) *found = 0;
    if (not_found) *not_found = 0;
    if (unknown) *unknown = 0;
    if (errors) *errors = 0;
    if (!store) return;

    for (int i = 0; i < store->count; i++) {
        switch (store->results[i].status) {
            case RESULT_FOUND: if (found) (*found)++; break;
            case RESULT_NOT_FOUND: if (not_found) (*not_found)++; break;
            case RESULT_UNKNOWN: if (unknown) (*unknown)++; break;
            case RESULT_ERROR: if (errors) (*errors)++; break;
        }
    }
}

void output_list_sites(const Site* sites, int count) {
    printf("%s%-4s %-26s %-16s %s%s\n", BOLD, "#", "Name", "Category", "NSFW", RESET);
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
    int found = 0;
    int not_found = 0;
    int unknown = 0;
    int errors = 0;
    const LowHuntMetadata* meta = lowhunt_metadata();

    if (!store || !cfg || cfg->output_file[0] == '\0') return;

    f = fopen(cfg->output_file, "w");
    if (!f) {
        fprintf(stderr, "%s[ERROR]%s Cannot open output file: %s\n", RED, RESET, cfg->output_file);
        return;
    }

    now = time(NULL);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%S%z", localtime(&now));
    summarize_counts(store, &found, &not_found, &unknown, &errors);

    if (strcmp(cfg->output_format, "json") == 0) {
        fprintf(f, "{\n  \"tool\": \"%s\",\n  \"version\": \"%s\",\n", meta->name, meta->version);
        fprintf(f, "  \"generated\": \"%s\",\n", timebuf);
        fprintf(f, "  \"preset\": \"%s\",\n", cfg->preset[0] ? cfg->preset : "balanced");
        fprintf(f, "  \"engine\": \"%s\",\n", cfg->engine[0] ? cfg->engine : "auto");
        fprintf(f, "  \"summary\": {\"found\": %d, \"not_found\": %d, \"unknown\": %d, \"errors\": %d},\n",
                found, not_found, unknown, errors);
        fprintf(f, "  \"results\": [\n");
        for (int i = 0; i < store->count; i++) {
            const ScanResult* r = &store->results[i];
            if (!cfg->very_verbose && r->status != RESULT_FOUND) continue;
            if (emitted++ > 0) fprintf(f, ",\n");
            fprintf(f,
                    "    {\"username\":\"%s\",\"site\":\"%s\",\"status\":\"%s\","
                    "\"url\":\"%s\",\"http_code\":%ld,\"response_time_ms\":%.2f}",
                    r->username, r->site_name, status_str(r->status),
                    r->url, r->http_code, r->response_time_ms);
        }
        fprintf(f, "\n  ]\n}\n");
    } else if (strcmp(cfg->output_format, "csv") == 0) {
        fprintf(f, "username,site,status,url,http_code,response_time_ms,preset,engine\n");
        for (int i = 0; i < store->count; i++) {
            const ScanResult* r = &store->results[i];
            if (!cfg->very_verbose && r->status != RESULT_FOUND) continue;
            fprintf(f, "\"%s\",\"%s\",\"%s\",\"%s\",%ld,%.2f,\"%s\",\"%s\"\n",
                    r->username, r->site_name, status_str(r->status),
                    r->url, r->http_code, r->response_time_ms,
                    cfg->preset[0] ? cfg->preset : "balanced",
                    cfg->engine[0] ? cfg->engine : "auto");
        }
    } else {
        fprintf(f, "%s %s | %s\n", meta->name, meta->version, timebuf);
        fprintf(f, "========================================\n\n");
        fprintf(f, "Preset: %s\n", cfg->preset[0] ? cfg->preset : "balanced");
        fprintf(f, "Engine: %s\n", cfg->engine[0] ? cfg->engine : "auto");
        fprintf(f, "Summary: found=%d not_found=%d unknown=%d errors=%d\n\n",
                found, not_found, unknown, errors);
        for (int i = 0; i < store->count; i++) {
            const ScanResult* r = &store->results[i];
            if (!cfg->very_verbose && r->status != RESULT_FOUND) continue;
            fprintf(f, "[%s] %-24s %s\n", status_str(r->status), r->site_name, r->url);
        }
    }

    fclose(f);
    printf("\n%s[+]%s Results written to: %s\n", GREEN, RESET, cfg->output_file);
}

void output_investigation(const ResultStore* store, const HarvestSummary* harvest,
                          const CorrelationSummary* correlation, const LowHuntConfig* cfg) {
    FILE* f;

    if (!store || !harvest || !correlation || !cfg || cfg->output_file[0] == '\0') return;
    f = fopen(cfg->output_file, "w");
    if (!f) {
        fprintf(stderr, "%s[ERROR]%s Cannot open output file: %s\n", RED, RESET, cfg->output_file);
        return;
    }

    if (strcmp(cfg->output_format, "json") == 0) {
        fprintf(f, "{\n  \"mode\": \"investigation\",\n  \"preset\": ");
        fprint_json_string(f, cfg->preset[0] ? cfg->preset : "balanced");
        fprintf(f, ",\n  \"engine\": ");
        fprint_json_string(f, cfg->engine[0] ? cfg->engine : "auto");
        fprintf(f, ",\n  \"domain\": ");
        fprint_json_string(f, harvest->domain);
        fprintf(f, ",\n  \"correlation\": {\n    \"confidence\": %d,\n    \"exact_matches\": %d,\n    \"partial_matches\": %d,\n    \"narrative\": ",
                correlation->confidence_score, correlation->exact_email_matches,
                correlation->partial_email_matches);
        fprint_json_string(f, correlation->narrative);
        fprintf(f, "\n  },\n  \"profiles\": [\n");
        for (int i = 0; i < store->count; i++) {
            const ScanResult* r = &store->results[i];
            fprintf(f, "    {\"username\":");
            fprint_json_string(f, r->username);
            fprintf(f, ",\"site\":");
            fprint_json_string(f, r->site_name);
            fprintf(f, ",\"status\":");
            fprint_json_string(f, status_str(r->status));
            fprintf(f, ",\"url\":");
            fprint_json_string(f, r->url);
            fprintf(f, "}%s\n", i + 1 < store->count ? "," : "");
        }
        fprintf(f, "  ],\n  \"hosts\": [\n");
        for (int i = 0; i < harvest->host_count; i++) {
            fprintf(f, "    {\"host\":");
            fprint_json_string(f, harvest->hosts[i].value);
            fprintf(f, ",\"sources\":");
            fprint_json_string(f, harvest->hosts[i].sources);
            fprintf(f, ",\"confidence\":%d}%s\n",
                    harvest->hosts[i].confidence, i + 1 < harvest->host_count ? "," : "");
        }
        fprintf(f, "  ],\n  \"emails\": [\n");
        for (int i = 0; i < harvest->email_count; i++) {
            fprintf(f, "    {\"email\":");
            fprint_json_string(f, harvest->emails[i].email);
            fprintf(f, ",\"page\":");
            fprint_json_string(f, harvest->emails[i].page);
            fprintf(f, ",\"confidence\":%d}%s\n",
                    harvest->emails[i].confidence, i + 1 < harvest->email_count ? "," : "");
        }
        fprintf(f, "  ]\n}\n");
    } else if (strcmp(cfg->output_format, "csv") == 0) {
        fprintf(f, "section,key,value,meta\n");
        fprintf(f, "correlation,confidence,%d,\n", correlation->confidence_score);
        fprintf(f, "correlation,exact_matches,%d,\n", correlation->exact_email_matches);
        fprintf(f, "correlation,partial_matches,%d,\n", correlation->partial_email_matches);
        for (int i = 0; i < store->count; i++) {
            fprintf(f, "profile,%s,%s,%s\n", store->results[i].username,
                    store->results[i].site_name, status_str(store->results[i].status));
        }
        for (int i = 0; i < harvest->host_count; i++) {
            fprintf(f, "host,%s,%s,%d\n", harvest->domain,
                    harvest->hosts[i].value, harvest->hosts[i].confidence);
        }
        for (int i = 0; i < harvest->email_count; i++) {
            fprintf(f, "email,%s,%s,%d\n", harvest->domain,
                    harvest->emails[i].email, harvest->emails[i].confidence);
        }
    } else {
        fprintf(f, "LowHunt investigation\n");
        fprintf(f, "==============================\n");
        fprintf(f, "Domain: %s\n", harvest->domain);
        fprintf(f, "Preset: %s\n", cfg->preset[0] ? cfg->preset : "balanced");
        fprintf(f, "Engine: %s\n", cfg->engine[0] ? cfg->engine : "auto");
        fprintf(f, "Correlation confidence: %d\n", correlation->confidence_score);
        fprintf(f, "Narrative: %s\n\n", correlation->narrative);
        fprintf(f, "Profiles:\n");
        for (int i = 0; i < store->count; i++) {
            const ScanResult* r = &store->results[i];
            if (!cfg->very_verbose && r->status != RESULT_FOUND) continue;
            fprintf(f, "  [%s] %s :: %s\n", status_str(r->status), r->site_name, r->url);
        }
        fprintf(f, "\nHosts:\n");
        for (int i = 0; i < harvest->host_count; i++) {
            fprintf(f, "  %s  [sources=%s confidence=%d]\n",
                    harvest->hosts[i].value, harvest->hosts[i].sources, harvest->hosts[i].confidence);
        }
        fprintf(f, "\nEmails:\n");
        for (int i = 0; i < harvest->email_count; i++) {
            fprintf(f, "  %s  <-  %s  [confidence=%d]\n",
                    harvest->emails[i].email, harvest->emails[i].page, harvest->emails[i].confidence);
        }
    }

    fclose(f);
    printf("\n%s[+]%s Investigation written to: %s\n", GREEN, RESET, cfg->output_file);
}
