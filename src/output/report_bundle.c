#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

#include "metadata.h"
#include "report_bundle.h"

static const char* status_str(ResultStatus s) {
    switch (s) {
        case RESULT_FOUND: return "FOUND";
        case RESULT_NOT_FOUND: return "NOT_FOUND";
        case RESULT_UNKNOWN: return "UNKNOWN";
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

static const char* home_dir(void) {
    const char* home = getenv("HOME");
    if (home && home[0]) return home;
#ifdef _WIN32
    home = getenv("USERPROFILE");
    if (home && home[0]) return home;
    home = getenv("LOCALAPPDATA");
    if (home && home[0]) return home;
#endif
    return ".";
}

static void sanitize_name(const char* in, char* out, size_t out_size) {
    size_t i = 0;

    if (!out || out_size == 0) return;
    while (in && *in && i + 1 < out_size) {
        char c = *in++;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_') {
            out[i++] = c;
        } else {
            out[i++] = '_';
        }
    }
    out[i] = '\0';
    if (i == 0 && out_size > 1) snprintf(out, out_size, "run");
}

static int mkdir_if_needed(const char* path) {
    if (!path || !path[0]) return -1;
    if (MKDIR(path) == 0) return 0;
    return 0;
}

static int ensure_report_dir(const char* target, char* out_dir, size_t out_size) {
    char base[512];
    char reports[640];
    char target_dir[768];
    char safe_target[128];
    char stamp[64];
    time_t now;
    struct tm* tm_now;

    sanitize_name(target, safe_target, sizeof(safe_target));
    snprintf(base, sizeof(base), "%s/.lowhunt", home_dir());
    snprintf(reports, sizeof(reports), "%s/output", base);
    snprintf(target_dir, sizeof(target_dir), "%s/reports/%s", reports, safe_target);

    now = time(NULL);
    tm_now = localtime(&now);
    strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", tm_now);

    mkdir_if_needed(base);
    mkdir_if_needed(reports);
    {
        char reports_root[768];
        snprintf(reports_root, sizeof(reports_root), "%s/reports", reports);
        mkdir_if_needed(reports_root);
    }
    mkdir_if_needed(target_dir);
    snprintf(out_dir, out_size, "%s/%s", target_dir, stamp);
    mkdir_if_needed(out_dir);
    return 0;
}

static void write_scan_cli(FILE* f, const ResultStore* store, const LowHuntConfig* cfg) {
    int found = 0;
    int unknown = 0;
    int errors = 0;

    for (int i = 0; i < store->count; i++) {
        if (store->results[i].status == RESULT_FOUND) found++;
        else if (store->results[i].status == RESULT_UNKNOWN) unknown++;
        else if (store->results[i].status == RESULT_ERROR) errors++;
    }

    fprintf(f, "LowHunt scan bundle\n");
    fprintf(f, "Preset: %s\n", cfg->preset[0] ? cfg->preset : "balanced");
    fprintf(f, "Engine: %s\n", cfg->engine[0] ? cfg->engine : "auto");
    fprintf(f, "Targets: %d\n", cfg->target_count);
    fprintf(f, "Summary: found=%d unknown=%d errors=%d total=%d\n\n",
            found, unknown, errors, store->count);
    for (int i = 0; i < store->count; i++) {
        const ScanResult* r = &store->results[i];
        if (!cfg->very_verbose && r->status != RESULT_FOUND) continue;
        fprintf(f, "[%s] %-24s %-20s %s\n",
                status_str(r->status), r->site_name, r->username, r->url);
    }
}

static void write_scan_json(FILE* f, const ResultStore* store, const LowHuntConfig* cfg) {
    const LowHuntMetadata* meta = lowhunt_metadata();

    fprintf(f, "{\n");
    fprintf(f, "  \"tool\": \"%s\",\n", meta->name);
    fprintf(f, "  \"version\": \"%s\",\n", meta->version);
    fprintf(f, "  \"preset\": \"%s\",\n", cfg->preset[0] ? cfg->preset : "balanced");
    fprintf(f, "  \"engine\": \"%s\",\n", cfg->engine[0] ? cfg->engine : "auto");
    fprintf(f, "  \"results\": [\n");
    for (int i = 0; i < store->count; i++) {
        const ScanResult* r = &store->results[i];
        fprintf(f,
                "    {\"username\":\"%s\",\"site\":\"%s\",\"status\":\"%s\",\"url\":\"%s\",\"http_code\":%ld,\"response_time_ms\":%.2f}%s\n",
                r->username, r->site_name, status_str(r->status), r->url,
                r->http_code, r->response_time_ms,
                i + 1 < store->count ? "," : "");
    }
    fprintf(f, "  ]\n}\n");
}

void report_bundle_write_scan(const ResultStore* store, const LowHuntConfig* cfg) {
    char dir[1024];
    char cli_path[1152];
    char txt_path[1152];
    char json_path[1152];
    FILE* f;
    const char* target = (cfg && cfg->target_count == 1) ? cfg->targets[0] : "batch";

    if (!store || !cfg) return;
    ensure_report_dir(target, dir, sizeof(dir));

    snprintf(cli_path, sizeof(cli_path), "%s/report.cli.txt", dir);
    snprintf(txt_path, sizeof(txt_path), "%s/report.txt", dir);
    snprintf(json_path, sizeof(json_path), "%s/report.json", dir);

    f = fopen(cli_path, "w");
    if (f) {
        write_scan_cli(f, store, cfg);
        fclose(f);
    }
    f = fopen(txt_path, "w");
    if (f) {
        write_scan_cli(f, store, cfg);
        fprintf(f, "\nGuidance:\n");
        fprintf(f, "- Treat FOUND hits as leads, not automatic attribution.\n");
        fprintf(f, "- Review UNKNOWN and ERROR results with a slower engine if the case is important.\n");
        fclose(f);
    }
    f = fopen(json_path, "w");
    if (f) {
        write_scan_json(f, store, cfg);
        fclose(f);
    }

    printf("\n[+] Stored report bundle: %s\n", dir);
}

void report_bundle_write_harvest(const HarvestSummary* summary, const LowHuntConfig* cfg) {
    char dir[1024];
    char cli_path[1152];
    char txt_path[1152];
    char json_path[1152];
    FILE* f;

    if (!summary || !cfg) return;
    ensure_report_dir(summary->domain[0] ? summary->domain : "harvest", dir, sizeof(dir));

    snprintf(cli_path, sizeof(cli_path), "%s/report.cli.txt", dir);
    snprintf(txt_path, sizeof(txt_path), "%s/report.txt", dir);
    snprintf(json_path, sizeof(json_path), "%s/report.json", dir);

    f = fopen(cli_path, "w");
    if (f) {
        fprintf(f, "LowHunt harvest bundle\n");
        fprintf(f, "Domain: %s\n", summary->domain);
        fprintf(f, "Sources: %s\n", summary->source_scope);
        fprintf(f, "Hosts: %d | Emails: %d | Pages scanned: %d\n\n",
                summary->host_count, summary->email_count, summary->pages_scanned);
        for (int i = 0; i < summary->host_count; i++) {
            fprintf(f, "[host] %s  [sources=%s confidence=%d]\n",
                    summary->hosts[i].value, summary->hosts[i].sources, summary->hosts[i].confidence);
        }
        for (int i = 0; i < summary->email_count; i++) {
            fprintf(f, "[email] %s  <-  %s  [confidence=%d]\n",
                    summary->emails[i].email, summary->emails[i].page, summary->emails[i].confidence);
        }
        fclose(f);
    }
    f = fopen(txt_path, "w");
    if (f) {
        fprintf(f, "LowHunt passive harvest report for %s\n", summary->domain);
        fprintf(f, "Sources used: %s\n", summary->source_scope);
        fprintf(f, "Public host overlap increases confidence.\n");
        fprintf(f, "Filtered noise email candidates: %d\n\n", summary->filtered_email_count);
        for (int i = 0; i < summary->host_count; i++) {
            fprintf(f, "Host: %s\nSources: %s\nConfidence: %d\n\n",
                    summary->hosts[i].value, summary->hosts[i].sources, summary->hosts[i].confidence);
        }
        fclose(f);
    }
    f = fopen(json_path, "w");
    if (f) {
        fprintf(f, "{\n  \"domain\": \"%s\",\n  \"sources\": \"%s\",\n", summary->domain, summary->source_scope);
        fprintf(f, "  \"hosts\": [\n");
        for (int i = 0; i < summary->host_count; i++) {
            fprintf(f,
                    "    {\"host\":\"%s\",\"sources\":\"%s\",\"source_count\":%d,\"confidence\":%d}%s\n",
                    summary->hosts[i].value,
                    summary->hosts[i].sources,
                    summary->hosts[i].source_count,
                    summary->hosts[i].confidence,
                    i + 1 < summary->host_count ? "," : "");
        }
        fprintf(f, "  ],\n  \"emails\": [\n");
        for (int i = 0; i < summary->email_count; i++) {
            fprintf(f,
                    "    {\"email\":\"%s\",\"page\":\"%s\",\"confidence\":%d}%s\n",
                    summary->emails[i].email,
                    summary->emails[i].page,
                    summary->emails[i].confidence,
                    i + 1 < summary->email_count ? "," : "");
        }
        fprintf(f, "  ],\n  \"pages_scanned\": %d,\n  \"filtered_email_candidates\": %d\n}\n",
                summary->pages_scanned, summary->filtered_email_count);
        fclose(f);
    }

    printf("\n[+] Stored report bundle: %s\n", dir);
}

void report_bundle_write_investigation(const ResultStore* store, const HarvestSummary* summary,
                                       const CorrelationSummary* correlation,
                                       const LowHuntConfig* cfg) {
    char dir[1024];
    char txt_path[1152];
    char json_path[1152];
    FILE* f;
    const char* target = (cfg && cfg->target_count == 1) ? cfg->targets[0] : "investigation";

    if (!store || !summary || !correlation || !cfg) return;
    ensure_report_dir(target, dir, sizeof(dir));

    snprintf(txt_path, sizeof(txt_path), "%s/report.txt", dir);
    snprintf(json_path, sizeof(json_path), "%s/report.json", dir);

    f = fopen(txt_path, "w");
    if (f) {
        fprintf(f, "LowHunt combined investigation report\n");
        fprintf(f, "Preset: %s\n", cfg->preset[0] ? cfg->preset : "balanced");
        fprintf(f, "Engine: %s\n", cfg->engine[0] ? cfg->engine : "auto");
        fprintf(f, "Domain: %s\n", summary->domain);
        fprintf(f, "Correlation confidence: %d\n", correlation->confidence_score);
        fprintf(f, "Narrative: %s\n\n", correlation->narrative);
        fprintf(f, "Profiles found: %d\n", correlation->found_profiles);
        fprintf(f, "Hosts: %d\n", summary->host_count);
        fprintf(f, "Emails: %d\n", summary->email_count);
        fprintf(f, "Exact username/email matches: %d\n", correlation->exact_email_matches);
        fprintf(f, "Loose username/email matches: %d\n\n", correlation->partial_email_matches);
        for (int i = 0; i < store->count; i++) {
            const ScanResult* r = &store->results[i];
            if (r->status != RESULT_FOUND && !cfg->very_verbose) continue;
            fprintf(f, "[%s] %s :: %s\n", status_str(r->status), r->site_name, r->url);
        }
        fclose(f);
    }

    f = fopen(json_path, "w");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "  \"domain\": ");
        fprint_json_string(f, summary->domain);
        fprintf(f, ",\n  \"correlation\": {\"confidence\": %d, \"narrative\": ",
                correlation->confidence_score);
        fprint_json_string(f, correlation->narrative);
        fprintf(f, ", \"exact_matches\": %d, \"partial_matches\": %d},\n",
                correlation->exact_email_matches, correlation->partial_email_matches);
        fprintf(f, "  \"hosts\": %d,\n", summary->host_count);
        fprintf(f, "  \"emails\": %d,\n", summary->email_count);
        fprintf(f, "  \"found_profiles\": %d\n", correlation->found_profiles);
        fprintf(f, "}\n");
        fclose(f);
    }

    printf("\n[+] Stored investigation bundle: %s\n", dir);
}
