#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "config.h"
#include "harvester.h"
#include "http.h"
#include "lowhunt.h"
#include "output.h"
#include "scanner.h"

static void print_banner(bool no_color) {
    const char* c = no_color ? "" : CYAN;
    const char* d = no_color ? "" : DIM;
    const char* r = no_color ? "" : RESET;

    printf("%s", c);
    printf("        _____  _  _  _ _     _ _     _ __   _ _______\n");
    printf("|      |     | |  |  | |_____| |     | | \\  |    |\n");
    printf("|_____ |_____| |__|__| |     | |_____| |  \\_|    |\n");
    printf("%s", r);
    printf("%s  v%s | Social Media & OSINT Recon | @%s | %s%s\n\n",
           d, LOWHUNT_VERSION, LOWHUNT_AUTHOR, LOWHUNT_EMAIL, r);
}

static void print_usage(const char* prog) {
    printf("Usage: %s [OPTIONS] <username> [username2 ...]\n\n", prog);
    printf("Username scanning:\n");
    printf("  -u, --username <name>     Target username. Can be repeated.\n");
    printf("  -f, --fast                Use data/sites_fast.json.\n");
    printf("  -s, --site <name>         Limit to one platform.\n");
    printf("      --nsfw                Include NSFW platforms.\n");
    printf("      --print-all           Print found and not-found results.\n\n");
    printf("Domain harvesting:\n");
    printf("  -d, --domain <domain>     Harvest public hostnames for domain.\n");
    printf("  -b, --source <source>     Source: crtsh or all. Default: crtsh.\n");
    printf("  -l, --limit <n>           Max harvest results. Default: 200.\n\n");
    printf("Network:\n");
    printf("      --tor                 Use socks5://127.0.0.1:9050.\n");
    printf("      --proxy <url>         Custom libcurl proxy URL.\n");
    printf("  -t, --timeout <ms>        Per-request timeout. Default: 10000.\n");
    printf("  -T, --threads <n>         Concurrent requests. Default: 20, max: 50.\n\n");
    printf("Output:\n");
    printf("  -o, --output <file>       Write output file.\n");
    printf("      --format <fmt>        txt, json, or csv. Default: txt.\n");
    printf("      --no-color            Disable ANSI colors.\n");
    printf("  -v, --verbose             Verbose output.\n\n");
    printf("Info:\n");
    printf("  -h, --help                Show help.\n");
    printf("      --version             Print version.\n");
    printf("      --list-sites          List supported platforms.\n");
}

static void add_target(LowHuntConfig* cfg, const char* username) {
    if (cfg->target_count >= MAX_TARGETS) return;
    snprintf(cfg->targets[cfg->target_count++], sizeof(cfg->targets[0]), "%s", username);
}

int main(int argc, char** argv) {
    LowHuntConfig cfg;
    ResultStore store;
    bool do_scan = false;
    bool do_harvest = false;
    bool list_sites = false;
    char domain[256] = {0};
    char source[128] = "crtsh";
    int limit = 200;
    int opt;
    int idx = 0;

    static struct option long_opts[] = {
        {"username", required_argument, 0, 'u'},
        {"fast", no_argument, 0, 'f'},
        {"site", required_argument, 0, 's'},
        {"nsfw", no_argument, 0, 1},
        {"print-all", no_argument, 0, 2},
        {"domain", required_argument, 0, 'd'},
        {"source", required_argument, 0, 'b'},
        {"limit", required_argument, 0, 'l'},
        {"tor", no_argument, 0, 3},
        {"proxy", required_argument, 0, 4},
        {"timeout", required_argument, 0, 't'},
        {"threads", required_argument, 0, 'T'},
        {"output", required_argument, 0, 'o'},
        {"format", required_argument, 0, 5},
        {"no-color", no_argument, 0, 6},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 7},
        {"list-sites", no_argument, 0, 8},
        {0, 0, 0, 0}
    };

    memset(&cfg, 0, sizeof(cfg));
    memset(&store, 0, sizeof(store));
    cfg.timeout_ms = DEFAULT_TIMEOUT_MS;
    cfg.thread_count = 20;
    snprintf(cfg.output_format, sizeof(cfg.output_format), "txt");

    while ((opt = getopt_long(argc, argv, "u:fs:d:b:l:t:T:o:vh", long_opts, &idx)) != -1) {
        switch (opt) {
            case 'u': add_target(&cfg, optarg); do_scan = true; break;
            case 'f': cfg.fast_scan = true; break;
            case 's': snprintf(cfg.site_filter, sizeof(cfg.site_filter), "%s", optarg); break;
            case 1: cfg.nsfw = true; break;
            case 2: cfg.print_all = true; break;
            case 'd': snprintf(domain, sizeof(domain), "%s", optarg); do_harvest = true; break;
            case 'b': snprintf(source, sizeof(source), "%s", optarg); break;
            case 'l': limit = atoi(optarg); break;
            case 3: snprintf(cfg.proxy, sizeof(cfg.proxy), "socks5://127.0.0.1:9050"); cfg.tor_enabled = true; break;
            case 4: snprintf(cfg.proxy, sizeof(cfg.proxy), "%s", optarg); break;
            case 't': cfg.timeout_ms = atoi(optarg); break;
            case 'T': cfg.thread_count = atoi(optarg); break;
            case 'o': snprintf(cfg.output_file, sizeof(cfg.output_file), "%s", optarg); break;
            case 5: snprintf(cfg.output_format, sizeof(cfg.output_format), "%s", optarg); break;
            case 6: cfg.no_color = true; break;
            case 'v': cfg.verbose = true; break;
            case 'h': print_banner(false); print_usage(argv[0]); return 0;
            case 7: printf("lowhunt %s\n", LOWHUNT_VERSION); return 0;
            case 8: list_sites = true; break;
            default: print_usage(argv[0]); return 1;
        }
    }

    while (optind < argc) {
        add_target(&cfg, argv[optind++]);
        do_scan = true;
    }

    if (cfg.thread_count < 1) cfg.thread_count = 1;
    if (cfg.thread_count > MAX_THREADS) cfg.thread_count = MAX_THREADS;
    if (cfg.timeout_ms < 1000) cfg.timeout_ms = 1000;

    print_banner(cfg.no_color);
    http_global_init();

    cfg.sites = config_load_sites(cfg.fast_scan ? "data/sites_fast.json" : "data/sites.json",
                                  &cfg.site_count);
    if ((do_scan || list_sites) && (!cfg.sites || cfg.site_count == 0)) {
        fprintf(stderr, "%s[ERROR]%s Could not load site database.\n",
                cfg.no_color ? "" : RED, cfg.no_color ? "" : RESET);
        http_global_cleanup();
        free(cfg.sites);
        return 1;
    }

    if (list_sites) {
        output_list_sites(cfg.sites, cfg.site_count, cfg.no_color);
    } else if (do_harvest) {
        harvester_run(domain, source, limit, &cfg);
    } else if (do_scan && cfg.target_count > 0) {
        pthread_mutex_init(&store.lock, NULL);
        scanner_run(&cfg, &store);
        output_results(&store, &cfg);
        pthread_mutex_destroy(&store.lock);
        free(store.results);
    } else {
        print_usage(argv[0]);
    }

    free(cfg.sites);
    http_global_cleanup();
    return 0;
}
