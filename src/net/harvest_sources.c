#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "harvest_sources.h"
#include "http.h"

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

static void trim_in_place(char* s) {
    char* start = s;
    char* end;

    if (!s) return;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);

    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
}

static void normalize_host(char* host) {
    if (!host) return;
    trim_in_place(host);
    while (*host == '*' || *host == '.') memmove(host, host + 1, strlen(host));
    for (size_t i = 0; host[i]; i++) {
        host[i] = (char)tolower((unsigned char)host[i]);
    }
}

static void emit_host(const char* raw, const char* source_name,
                      const char* domain, HarvestHostSink sink, void* userdata) {
    char host[512];

    if (!raw || !source_name || !domain || !sink) return;
    snprintf(host, sizeof(host), "%s", raw);
    normalize_host(host);
    if (!host[0]) return;
    if (!contains_domain(host, domain)) return;
    sink(host, source_name, userdata);
}

static int collect_crtsh(const char* domain, const LowHuntConfig* cfg,
                         HarvestHostSink sink, void* userdata) {
    char url[512];
    HttpResult* response;
    const char* key = "\"name_value\"";
    const char* p;
    int added = 0;

    snprintf(url, sizeof(url),
             "https://crt.sh/?q=%%25.%s&exclude=expired&deduplicate=Y&output=json",
             domain);
    response = http_get(url, cfg->timeout_ms, DEFAULT_USER_AGENT, cfg->proxy);
    if (!response || !response->success || !response->body.data) {
        http_result_free(response);
        return 0;
    }

    p = response->body.data;
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
            emit_host(line, "crtsh", domain, sink, userdata);
            added++;
        }
        p = second + 1;
    }

    http_result_free(response);
    return added;
}

static int collect_rapiddns(const char* domain, const LowHuntConfig* cfg,
                            HarvestHostSink sink, void* userdata) {
    char url[512];
    HttpResult* response;
    const char* p;
    int added = 0;

    snprintf(url, sizeof(url), "https://rapiddns.io/subdomain/%s?full=1#result", domain);
    response = http_get(url, cfg->timeout_ms, DEFAULT_USER_AGENT, cfg->proxy);
    if (!response || !response->success || !response->body.data) {
        http_result_free(response);
        return 0;
    }

    p = response->body.data;
    while ((p = strstr(p, "/subdomain/")) != NULL) {
        const char* start = p + strlen("/subdomain/");
        const char* end = strchr(start, '"');
        char host[512];
        size_t len;

        if (!end) break;
        len = (size_t)(end - start);
        if (len == 0 || len >= sizeof(host)) {
            p = start;
            continue;
        }
        memcpy(host, start, len);
        host[len] = '\0';
        emit_host(host, "rapiddns", domain, sink, userdata);
        added++;
        p = end + 1;
    }

    http_result_free(response);
    return added;
}

static void extract_host_from_url(const char* url, char* out, size_t out_size) {
    const char* start;
    const char* end;
    size_t len;

    if (!url || !out || out_size == 0) return;
    out[0] = '\0';
    start = strstr(url, "://");
    start = start ? start + 3 : url;
    end = strpbrk(start, "/?#");
    len = end ? (size_t)(end - start) : strlen(start);
    if (len == 0 || len >= out_size) return;
    memcpy(out, start, len);
    out[len] = '\0';
}

static int collect_wayback(const char* domain, const LowHuntConfig* cfg,
                           HarvestHostSink sink, void* userdata) {
    char url[768];
    HttpResult* response;
    const char* p;
    int added = 0;

    snprintf(url, sizeof(url),
             "https://web.archive.org/cdx/search/cdx?url=*.%s/*&output=json&fl=original&collapse=urlkey",
             domain);
    response = http_get(url, cfg->timeout_ms, DEFAULT_USER_AGENT, cfg->proxy);
    if (!response || !response->success || !response->body.data) {
        http_result_free(response);
        return 0;
    }

    p = response->body.data;
    while ((p = strchr(p, '"')) != NULL) {
        const char* end = strchr(p + 1, '"');
        char value[768];
        char host[512];
        size_t len;

        if (!end) break;
        len = (size_t)(end - p - 1);
        if (len > 0 && len < sizeof(value)) {
            memcpy(value, p + 1, len);
            value[len] = '\0';
            if (strstr(value, "://")) {
                extract_host_from_url(value, host, sizeof(host));
                if (host[0]) {
                    emit_host(host, "wayback", domain, sink, userdata);
                    added++;
                }
            }
        }
        p = end + 1;
    }

    http_result_free(response);
    return added;
}

int harvest_source_collect(const char* source_name, const char* domain,
                           const LowHuntConfig* cfg, HarvestHostSink sink,
                           void* userdata) {
    if (!source_name || !domain || !cfg || !sink) return 0;

    if (strcasecmp(source_name, "crtsh") == 0) {
        return collect_crtsh(domain, cfg, sink, userdata);
    }
    if (strcasecmp(source_name, "rapiddns") == 0) {
        return collect_rapiddns(domain, cfg, sink, userdata);
    }
    if (strcasecmp(source_name, "wayback") == 0) {
        return collect_wayback(domain, cfg, sink, userdata);
    }
    if (strcasecmp(source_name, "all") == 0) {
        return collect_crtsh(domain, cfg, sink, userdata) +
               collect_rapiddns(domain, cfg, sink, userdata) +
               collect_wayback(domain, cfg, sink, userdata);
    }
    return 0;
}

void harvest_source_describe_all(void) {
    printf("%-14s %s\n", "crtsh", "Certificate-transparency hostname discovery.");
    printf("%-14s %s\n", "rapiddns", "Public passive subdomain listing from RapidDNS.");
    printf("%-14s %s\n", "wayback", "Wayback CDX hostname extraction from archived URLs.");
    printf("%-14s %s\n", "all", "Run all wired public passive host sources and fuse overlaps.");
}
