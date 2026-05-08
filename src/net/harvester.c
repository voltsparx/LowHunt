#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "colors.h"
#include "engine.h"
#include "harvester.h"
#include "harvest_sources.h"
#include "http.h"

#define MAX_CONTACT_URLS 48

typedef struct {
    HarvestSummary summary;
    int host_capacity;
    int email_capacity;
} HarvestResult;

typedef struct {
    HarvestResult* result;
    char domain[256];
    char url[512];
} HarvestTaskData;

static const char* case_strstr(const char* haystack, const char* needle) {
    size_t needle_len;

    if (!haystack || !needle || !needle[0]) return haystack;
    needle_len = strlen(needle);
    for (const char* p = haystack; *p; p++) {
        size_t i = 0;
        while (i < needle_len &&
               p[i] &&
               tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i])) {
            i++;
        }
        if (i == needle_len) return p;
        if (!p[i]) return NULL;
    }
    return NULL;
}

static bool contains_domain(const char* host, const char* domain) {
    size_t hlen;
    size_t dlen;

    if (!host || !domain) return false;
    hlen = strlen(host);
    dlen = strlen(domain);
    if (hlen < dlen) return false;
    if (strcasecmp(host, domain) == 0) return true;
    return hlen > dlen && host[hlen - dlen - 1] == '.' &&
           strcasecmp(host + hlen - dlen, domain) == 0;
}

static void lowercase_copy(char* out, size_t out_size, const char* in) {
    size_t i = 0;
    if (!out || out_size == 0) return;
    while (in && in[i] && i + 1 < out_size) {
        out[i] = (char)tolower((unsigned char)in[i]);
        i++;
    }
    out[i] = '\0';
}

static void ensure_host_capacity(HarvestResult* result) {
    HarvestHostRecord* next;
    int next_capacity;

    if (!result) return;
    if (result->summary.host_count < result->host_capacity) return;

    next_capacity = result->host_capacity == 0 ? 64 : result->host_capacity * 2;
    next = realloc(result->summary.hosts, (size_t)next_capacity * sizeof(HarvestHostRecord));
    if (!next) return;
    result->summary.hosts = next;
    result->host_capacity = next_capacity;
}

static void ensure_email_capacity(HarvestResult* result) {
    HarvestEmailRecord* next;
    int next_capacity;

    if (!result) return;
    if (result->summary.email_count < result->email_capacity) return;

    next_capacity = result->email_capacity == 0 ? 32 : result->email_capacity * 2;
    next = realloc(result->summary.emails, (size_t)next_capacity * sizeof(HarvestEmailRecord));
    if (!next) return;
    result->summary.emails = next;
    result->email_capacity = next_capacity;
}

static int compute_host_confidence(int source_count) {
    int confidence = 45;

    if (source_count >= 2) confidence += 25;
    if (source_count >= 3) confidence += 20;
    if (confidence > 95) confidence = 95;
    return confidence;
}

static int compute_email_confidence(const char* email, const char* page) {
    int confidence = 60;
    char lower_email[256];
    char lower_page[512];

    lowercase_copy(lower_email, sizeof(lower_email), email ? email : "");
    lowercase_copy(lower_page, sizeof(lower_page), page ? page : "");
    if (strstr(lower_page, "contact") || strstr(lower_page, "support") ||
        strstr(lower_page, "team") || strstr(lower_page, "about")) {
        confidence += 15;
    }
    if (strncmp(lower_email, "contact@", 8) == 0 ||
        strncmp(lower_email, "info@", 5) == 0 ||
        strncmp(lower_email, "support@", 8) == 0 ||
        strncmp(lower_email, "security@", 9) == 0 ||
        strncmp(lower_email, "privacy@", 8) == 0) {
        confidence += 10;
    }
    if (confidence > 95) confidence = 95;
    return confidence;
}

static void append_source_tag(char* buffer, size_t size, const char* source_name) {
    size_t current;

    if (!buffer || size == 0 || !source_name || !source_name[0]) return;
    if (strstr(buffer, source_name) != NULL) return;

    current = strlen(buffer);
    if (current > 0 && current + 1 < size) {
        buffer[current++] = ',';
        buffer[current] = '\0';
    }
    if (current + strlen(source_name) + 1 < size) {
        strcat(buffer, source_name);
    }
}

static void add_host_with_source(HarvestResult* result, const char* value, const char* source_name) {
    char normalized[512];

    if (!result || !value || !value[0]) return;
    lowercase_copy(normalized, sizeof(normalized), value);

    for (int i = 0; i < result->summary.host_count; i++) {
        if (strcasecmp(result->summary.hosts[i].value, normalized) == 0) {
            int before = result->summary.hosts[i].source_count;
            append_source_tag(result->summary.hosts[i].sources,
                              sizeof(result->summary.hosts[i].sources),
                              source_name);
            if (before == result->summary.hosts[i].source_count) {
                /* no-op path preserved by next recompute */
            }
            {
                int count = 1;
                for (const char* p = result->summary.hosts[i].sources; *p; p++) {
                    if (*p == ',') count++;
                }
                result->summary.hosts[i].source_count = count;
                result->summary.hosts[i].confidence = compute_host_confidence(count);
            }
            return;
        }
    }

    ensure_host_capacity(result);
    if (result->summary.host_count >= result->host_capacity) return;

    memset(&result->summary.hosts[result->summary.host_count], 0, sizeof(HarvestHostRecord));
    snprintf(result->summary.hosts[result->summary.host_count].value,
             sizeof(result->summary.hosts[result->summary.host_count].value),
             "%s", normalized);
    snprintf(result->summary.hosts[result->summary.host_count].sources,
             sizeof(result->summary.hosts[result->summary.host_count].sources),
             "%s", source_name ? source_name : "unknown");
    result->summary.hosts[result->summary.host_count].source_count = 1;
    result->summary.hosts[result->summary.host_count].confidence = compute_host_confidence(1);
    result->summary.host_count++;
}

static void on_passive_host(const char* host, const char* source_name, void* userdata) {
    HarvestResult* result = (HarvestResult*)userdata;
    add_host_with_source(result, host, source_name);
}

static bool url_has_asset_suffix(const char* s) {
    static const char* bad_suffixes[] = {
        ".png",".jpg",".jpeg",".gif",".svg",".webp",".css",".js",".map",
        ".ico",".woff",".woff2",".ttf",".pdf",".xml",".json",NULL
    };
    char lower[256];

    lowercase_copy(lower, sizeof(lower), s);
    for (int i = 0; bad_suffixes[i]; i++) {
        size_t suffix_len = strlen(bad_suffixes[i]);
        size_t value_len = strlen(lower);
        if (value_len >= suffix_len &&
            strcmp(lower + value_len - suffix_len, bad_suffixes[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool email_has_noise(const char* email) {
    static const char* bad_prefixes[] = {
        "noreply@", "no-reply@", "donotreply@", "do-not-reply@", "example@", NULL
    };
    char lower[256];

    lowercase_copy(lower, sizeof(lower), email);
    for (int i = 0; bad_prefixes[i]; i++) {
        if (strncmp(lower, bad_prefixes[i], strlen(bad_prefixes[i])) == 0) return true;
    }
    if (strstr(lower, "..")) return true;
    if (strstr(lower, "@localhost")) return true;
    if (url_has_asset_suffix(lower)) return true;
    return false;
}

static bool is_valid_same_domain_email(const char* email, const char* domain) {
    const char* at;
    const char* dot;
    size_t local_len;
    char domain_part[256];

    if (!email || !domain) return false;
    at = strchr(email, '@');
    if (!at || at == email || !at[1]) return false;
    local_len = (size_t)(at - email);
    if (local_len == 0 || local_len >= 128) return false;
    if (email[0] == '.' || at[-1] == '.') return false;

    snprintf(domain_part, sizeof(domain_part), "%s", at + 1);
    dot = strchr(domain_part, '.');
    if (!dot || !dot[1]) return false;
    if (!contains_domain(domain_part, domain)) return false;
    if (email_has_noise(email)) return false;
    return true;
}

static void add_unique_email(HarvestResult* result, const char* email, const char* page) {
    if (!result || !email || !email[0]) return;

    for (int i = 0; i < result->summary.email_count; i++) {
        if (strcasecmp(result->summary.emails[i].email, email) == 0) return;
    }

    ensure_email_capacity(result);
    if (result->summary.email_count >= result->email_capacity) return;

    memset(&result->summary.emails[result->summary.email_count], 0, sizeof(HarvestEmailRecord));
    snprintf(result->summary.emails[result->summary.email_count].email,
             sizeof(result->summary.emails[result->summary.email_count].email),
             "%s", email);
    snprintf(result->summary.emails[result->summary.email_count].page,
             sizeof(result->summary.emails[result->summary.email_count].page),
             "%s", page ? page : "");
    result->summary.emails[result->summary.email_count].confidence =
        compute_email_confidence(email, page);
    result->summary.email_count++;
}

static void extract_emails_from_body(const char* body, const char* page,
                                     const char* domain, HarvestResult* result) {
    size_t len;

    if (!body || !domain || !result) return;
    len = strlen(body);
    for (size_t i = 0; i < len; i++) {
        size_t start = i;
        size_t end = i;
        bool seen_at = false;
        char candidate[256];
        size_t out_len;

        if (!isalnum((unsigned char)body[i]) &&
            body[i] != '.' && body[i] != '_' && body[i] != '%' &&
            body[i] != '+' && body[i] != '-') {
            continue;
        }

        while (start > 0) {
            char c = body[start - 1];
            if (isalnum((unsigned char)c) || c == '.' || c == '_' ||
                c == '%' || c == '+' || c == '-') {
                start--;
            } else {
                break;
            }
        }

        while (end < len) {
            char c = body[end];
            if (isalnum((unsigned char)c) || c == '.' || c == '_' ||
                c == '%' || c == '+' || c == '-' || c == '@') {
                if (c == '@') seen_at = true;
                end++;
            } else {
                break;
            }
        }

        if (!seen_at || end <= start) continue;
        out_len = end - start;
        if (out_len == 0 || out_len >= sizeof(candidate)) continue;

        memcpy(candidate, body + start, out_len);
        candidate[out_len] = '\0';

        if (is_valid_same_domain_email(candidate, domain)) {
            add_unique_email(result, candidate, page);
        } else if (strchr(candidate, '@')) {
            result->summary.filtered_email_count++;
        }

        i = end;
    }
}

static bool is_contactish_path(const char* path) {
    static const char* keywords[] = {
        "contact", "about", "support", "team", "company", "legal", "privacy",
        "impressum", "help", "staff", NULL
    };
    char lower[512];

    if (!path || !path[0]) return false;
    lowercase_copy(lower, sizeof(lower), path);
    for (int i = 0; keywords[i]; i++) {
        if (strstr(lower, keywords[i])) return true;
    }
    return false;
}

static bool extract_same_domain_url(const char* href, const char* domain,
                                    char* out, size_t out_size) {
    if (!href || !href[0] || !domain || !out || out_size == 0) return false;

    if (strncmp(href, "mailto:", 7) == 0 || href[0] == '#') return false;
    if (strncmp(href, "http://", 7) == 0 || strncmp(href, "https://", 8) == 0) {
        const char* host = strstr(href, "://");
        const char* path;
        char host_buf[256];
        size_t host_len;

        if (!host) return false;
        host += 3;
        path = strchr(host, '/');
        host_len = path ? (size_t)(path - host) : strlen(host);
        if (host_len == 0 || host_len >= sizeof(host_buf)) return false;
        memcpy(host_buf, host, host_len);
        host_buf[host_len] = '\0';
        if (!contains_domain(host_buf, domain)) return false;
        snprintf(out, out_size, "%s", href);
        return true;
    }

    if (href[0] == '/') {
        snprintf(out, out_size, "https://%s%s", domain, href);
        return true;
    }
    return false;
}

static void add_unique_url(char urls[][512], int* count, const char* url) {
    if (!urls || !count || !url || !url[0]) return;
    for (int i = 0; i < *count; i++) {
        if (strcasecmp(urls[i], url) == 0) return;
    }
    if (*count >= MAX_CONTACT_URLS) return;
    snprintf(urls[*count], 512, "%s", url);
    (*count)++;
}

static void extract_contact_links(const char* body, const char* domain,
                                  char urls[][512], int* count) {
    const char* p = body;

    while ((p = case_strstr(p, "href=")) != NULL) {
        const char* start = NULL;
        const char* end = NULL;
        char href[512];
        char normalized[512];

        p += 5;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '"' || *p == '\'') {
            char quote = *p++;
            start = p;
            end = strchr(start, quote);
        }
        if (!start || !end) continue;
        if ((size_t)(end - start) >= sizeof(href)) {
            p = end + 1;
            continue;
        }
        memcpy(href, start, (size_t)(end - start));
        href[end - start] = '\0';
        if (is_contactish_path(href) &&
            extract_same_domain_url(href, domain, normalized, sizeof(normalized))) {
            add_unique_url(urls, count, normalized);
        }
        p = end + 1;
    }
}

static void harvest_page_done(HttpResult* http, void* userdata) {
    HarvestTaskData* data = (HarvestTaskData*)userdata;

    if (!data || !data->result) return;
    data->result->summary.pages_scanned++;
    if (!http || !http->success) return;
    if (http->http_code < 200 || http->http_code >= 500) return;
    extract_emails_from_body(http->body.data, data->url, data->domain, data->result);
}

static EngineType choose_harvest_engine(const LowHuntConfig* cfg, int task_count) {
    if (cfg && cfg->engine[0] && strcasecmp(cfg->engine, "auto") != 0) {
        return engine_from_str(cfg->engine);
    }
    if (cfg && (cfg->tor_enabled || cfg->proxy[0])) return ENGINE_STABILIZER;
    if (task_count <= 4) return ENGINE_SYNC;
    if (task_count >= 16 && cfg && cfg->thread_count >= 16) return ENGINE_FUSION;
    if (task_count >= 8) return ENGINE_ASYNC;
    return ENGINE_THREADPOOL;
}

static void run_contact_collection(const char* domain, const LowHuntConfig* cfg,
                                   HarvestResult* result) {
    static const char* seed_paths[] = {
        "/", "/contact", "/about", "/support", "/team", "/company",
        "/legal", "/privacy", "/help", "/impressum", NULL
    };
    char seed_urls[MAX_CONTACT_URLS][512] = {{0}};
    char extra_urls[MAX_CONTACT_URLS][512] = {{0}};
    HarvestTaskData task_data[MAX_CONTACT_URLS];
    HttpTask tasks[MAX_CONTACT_URLS];
    int seed_count = 0;
    int extra_count = 0;
    Engine* engine;
    EngineType type;

    for (int i = 0; seed_paths[i]; i++) {
        char url[512];
        snprintf(url, sizeof(url), "https://%s%s", domain, seed_paths[i]);
        add_unique_url(seed_urls, &seed_count, url);
    }

    type = choose_harvest_engine(cfg, seed_count);
    engine = engine_create(type);
    if (!engine || !engine->run) {
        engine_free(engine);
        engine = engine_create(ENGINE_THREADPOOL);
    }
    if (!engine || !engine->run) return;

    for (int i = 0; i < seed_count; i++) {
        memset(&task_data[i], 0, sizeof(task_data[i]));
        memset(&tasks[i], 0, sizeof(tasks[i]));
        task_data[i].result = result;
        snprintf(task_data[i].domain, sizeof(task_data[i].domain), "%s", domain);
        snprintf(task_data[i].url, sizeof(task_data[i].url), "%s", seed_urls[i]);
        snprintf(tasks[i].url, sizeof(tasks[i].url), "%s", seed_urls[i]);
        tasks[i].userdata = &task_data[i];
        tasks[i].callback = harvest_page_done;
    }

    if (cfg->verbose) {
        printf("%s[INFO]%s Collecting public contact pages with engine '%s'...\n",
               BLUE, RESET, engine->name);
    }
    engine->run(engine, tasks, seed_count, cfg);

    for (int i = 0; i < seed_count; i++) {
        HttpResult* page = http_get(seed_urls[i], cfg->timeout_ms, DEFAULT_USER_AGENT, cfg->proxy);
        if (page && page->success && page->http_code >= 200 && page->http_code < 500) {
            extract_contact_links(page->body.data, domain, extra_urls, &extra_count);
        }
        http_result_free(page);
    }

    for (int i = 0; i < extra_count; i++) {
        memset(&task_data[i], 0, sizeof(task_data[i]));
        memset(&tasks[i], 0, sizeof(tasks[i]));
        task_data[i].result = result;
        snprintf(task_data[i].domain, sizeof(task_data[i].domain), "%s", domain);
        snprintf(task_data[i].url, sizeof(task_data[i].url), "%s", extra_urls[i]);
        snprintf(tasks[i].url, sizeof(tasks[i].url), "%s", extra_urls[i]);
        tasks[i].userdata = &task_data[i];
        tasks[i].callback = harvest_page_done;
    }

    if (extra_count > 0) {
        if (cfg->verbose) {
            printf("%s[INFO]%s Following %d discovered contact-oriented link(s)...\n",
                   BLUE, RESET, extra_count);
        }
        engine->run(engine, tasks, extra_count, cfg);
    }

    engine_free(engine);
}

static int compare_hosts_desc(const void* a, const void* b) {
    const HarvestHostRecord* left = (const HarvestHostRecord*)a;
    const HarvestHostRecord* right = (const HarvestHostRecord*)b;

    if (left->confidence != right->confidence) {
        return right->confidence - left->confidence;
    }
    return strcasecmp(left->value, right->value);
}

static int compare_emails_desc(const void* a, const void* b) {
    const HarvestEmailRecord* left = (const HarvestEmailRecord*)a;
    const HarvestEmailRecord* right = (const HarvestEmailRecord*)b;

    if (left->confidence != right->confidence) {
        return right->confidence - left->confidence;
    }
    return strcasecmp(left->email, right->email);
}

static void write_harvest_output(const HarvestSummary* summary, int host_limit,
                                 const LowHuntConfig* cfg) {
    FILE* f;

    if (!cfg || cfg->output_file[0] == '\0' || !summary) return;
    f = fopen(cfg->output_file, "w");
    if (!f) {
        fprintf(stderr, "%s[ERROR]%s Cannot open output file: %s\n", RED, RESET, cfg->output_file);
        return;
    }

    if (strcmp(cfg->output_format, "json") == 0) {
        fprintf(f, "{\n  \"domain\": \"%s\",\n", summary->domain);
        fprintf(f, "  \"sources\": \"%s\",\n", summary->source_scope);
        fprintf(f, "  \"hosts\": [\n");
        for (int i = 0; i < host_limit; i++) {
            fprintf(f,
                    "    {\"host\":\"%s\",\"sources\":\"%s\",\"source_count\":%d,\"confidence\":%d}%s\n",
                    summary->hosts[i].value,
                    summary->hosts[i].sources,
                    summary->hosts[i].source_count,
                    summary->hosts[i].confidence,
                    i + 1 < host_limit ? "," : "");
        }
        fprintf(f, "  ],\n  \"emails\": [\n");
        for (int i = 0; i < summary->email_count; i++) {
            fprintf(f,
                    "    {\"email\":\"%s\",\"source\":\"%s\",\"confidence\":%d}%s\n",
                    summary->emails[i].email,
                    summary->emails[i].page,
                    summary->emails[i].confidence,
                    i + 1 < summary->email_count ? "," : "");
        }
        fprintf(f, "  ],\n");
        fprintf(f, "  \"pages_scanned\": %d,\n", summary->pages_scanned);
        fprintf(f, "  \"filtered_email_candidates\": %d\n}\n", summary->filtered_email_count);
    } else if (strcmp(cfg->output_format, "csv") == 0) {
        fprintf(f, "type,domain,value,source,source_count,confidence\n");
        for (int i = 0; i < host_limit; i++) {
            fprintf(f, "host,%s,%s,%s,%d,%d\n",
                    summary->domain,
                    summary->hosts[i].value,
                    summary->hosts[i].sources,
                    summary->hosts[i].source_count,
                    summary->hosts[i].confidence);
        }
        for (int i = 0; i < summary->email_count; i++) {
            fprintf(f, "email,%s,%s,%s,,%d\n",
                    summary->domain,
                    summary->emails[i].email,
                    summary->emails[i].page,
                    summary->emails[i].confidence);
        }
    } else {
        fprintf(f, "LowHunt harvest for %s\n", summary->domain);
        fprintf(f, "========================================\n");
        fprintf(f, "Sources: %s\n", summary->source_scope);
        fprintf(f, "Hosts (%d):\n", host_limit);
        for (int i = 0; i < host_limit; i++) {
            fprintf(f, "  %s  [sources=%s confidence=%d]\n",
                    summary->hosts[i].value,
                    summary->hosts[i].sources,
                    summary->hosts[i].confidence);
        }
        fprintf(f, "\nEmails (%d):\n", summary->email_count);
        for (int i = 0; i < summary->email_count; i++) {
            fprintf(f, "  %s  <-  %s  [confidence=%d]\n",
                    summary->emails[i].email,
                    summary->emails[i].page,
                    summary->emails[i].confidence);
        }
        fprintf(f, "\nPages scanned: %d\n", summary->pages_scanned);
        fprintf(f, "Filtered noise candidates: %d\n", summary->filtered_email_count);
    }
    fclose(f);
    printf("\n%s[+]%s Harvest written to: %s\n", GREEN, RESET, cfg->output_file);
}

void harvester_print_sources(void) {
    printf("%sHarvest Sources%s\n\n", BOLD, RESET);
    harvest_source_describe_all();
    printf("\n%sOnly public, passive collection paths are supported.%s\n", YELLOW, RESET);
}

void harvester_summary_free(HarvestSummary* summary) {
    if (!summary) return;
    free(summary->hosts);
    free(summary->emails);
    memset(summary, 0, sizeof(*summary));
}

void harvester_run(const char* domain, const char* source, int limit,
                   const LowHuntConfig* cfg, HarvestSummary* summary_out) {
    HarvestResult result;
    const char* selected = (source && source[0]) ? source : "all";
    int display_hosts;

    memset(&result, 0, sizeof(result));
    if (summary_out) memset(summary_out, 0, sizeof(*summary_out));
    if (!domain || !domain[0] || !cfg) return;

    snprintf(result.summary.domain, sizeof(result.summary.domain), "%s", domain);
    snprintf(result.summary.source_scope, sizeof(result.summary.source_scope), "%s", selected);

    if (cfg->verbose) {
        printf("Harvesting passive public intelligence for %s with source scope '%s'...\n\n",
               domain, selected);
    }

    if (harvest_source_collect(selected, domain, cfg, on_passive_host, &result) == 0 &&
        strcasecmp(selected, "crtsh") != 0 &&
        strcasecmp(selected, "all") != 0 &&
        strcasecmp(selected, "rapiddns") != 0 &&
        strcasecmp(selected, "wayback") != 0) {
        printf("%s[!]%s Source '%s' is not implemented yet; using all wired passive sources.\n",
               YELLOW, RESET, selected);
        snprintf(result.summary.source_scope, sizeof(result.summary.source_scope), "%s", "all");
        harvest_source_collect("all", domain, cfg, on_passive_host, &result);
    }

    run_contact_collection(domain, cfg, &result);

    qsort(result.summary.hosts, (size_t)result.summary.host_count,
          sizeof(HarvestHostRecord), compare_hosts_desc);
    qsort(result.summary.emails, (size_t)result.summary.email_count,
          sizeof(HarvestEmailRecord), compare_emails_desc);

    display_hosts = result.summary.host_count;
    if (limit > 0 && display_hosts > limit) display_hosts = limit;

    printf("%s[hosts]%s %d public hostname(s)\n", CYAN, RESET, display_hosts);
    for (int i = 0; i < display_hosts; i++) {
        printf("%s[+]%s %s", GREEN, RESET, result.summary.hosts[i].value);
        if (cfg->verbose || cfg->very_verbose) {
            printf("  [sources=%s confidence=%d]",
                   result.summary.hosts[i].sources,
                   result.summary.hosts[i].confidence);
        }
        printf("\n");
    }

    printf("\n%s[emails]%s %d same-domain contact email(s)\n", CYAN, RESET, result.summary.email_count);
    for (int i = 0; i < result.summary.email_count; i++) {
        if (!cfg->very_verbose && i >= 10) {
            printf("%s[INFO]%s Showing first 10 emails only. Use -vv for the full on-screen list.\n",
                   BLUE, RESET);
            break;
        }
        printf("%s[@]%s %s", GREEN, RESET, result.summary.emails[i].email);
        if (cfg->verbose || cfg->very_verbose) {
            printf("  <-  %s  [confidence=%d]",
                   result.summary.emails[i].page,
                   result.summary.emails[i].confidence);
        }
        printf("\n");
    }

    printf("\nSources used: %s\n", result.summary.source_scope);
    printf("Pages scanned: %d\n", result.summary.pages_scanned);
    printf("Filtered noise candidates: %d\n", result.summary.filtered_email_count);
    write_harvest_output(&result.summary, display_hosts, cfg);

    if (summary_out) {
        *summary_out = result.summary;
    } else {
        harvester_summary_free(&result.summary);
    }
}
