#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "banner.h"
#include "colors.h"
#include "config.h"
#include "engine.h"
#include "harvester.h"
#include "help_menu.h"
#include "http.h"
#include "lowhunt.h"
#include "metadata.h"
#include "correlation.h"
#include "output.h"
#include "report_bundle.h"
#include "resource_guard.h"
#include "scanner.h"

static void add_target(LowHuntConfig* cfg, const char* username) {
    if (!cfg || !username || cfg->target_count >= MAX_TARGETS) return;
    snprintf(cfg->targets[cfg->target_count++], sizeof(cfg->targets[0]), "%s", username);
}

static void apply_preset(LowHuntConfig* cfg, const char* preset) {
    if (!cfg || !preset || !preset[0]) return;

    if (strcasecmp(preset, "beginner") == 0) {
        cfg->verbose = true;
        cfg->very_verbose = false;
        cfg->mod_intelligence = true;
        if (cfg->thread_count < 12) cfg->thread_count = 12;
        if (strcasecmp(cfg->engine, "auto") == 0) {
            snprintf(cfg->engine, sizeof(cfg->engine), "stabilizer");
        }
    } else if (strcasecmp(preset, "aggressive") == 0) {
        cfg->verbose = true;
        cfg->very_verbose = true;
        cfg->mod_intelligence = true;
        if (cfg->thread_count < 64) cfg->thread_count = 64;
        if (strcasecmp(cfg->engine, "auto") == 0) {
            snprintf(cfg->engine, sizeof(cfg->engine), "fusion");
        }
    } else {
        snprintf(cfg->engine, sizeof(cfg->engine), "%s", cfg->engine[0] ? cfg->engine : "auto");
    }
}

int main(int argc, char** argv) {
    const LowHuntMetadata* meta = lowhunt_metadata();
    LowHuntConfig cfg;
    ResultStore store;
    bool do_scan = false;
    bool do_harvest = false;
    bool list_sites = false;
    bool list_sources = false;
    bool show_about = false;
    char domain[256] = {0};
    char source[128] = "crtsh";
    char explain_topic[64] = {0};
    CorrelationSummary correlation;
    HarvestSummary harvest_summary;
    int limit = 200;
    int opt;
    int idx = 0;
    int verbosity = 0;

    static struct option long_opts[] = {
        {"username", required_argument, 0, 'u'},
        {"fast", no_argument, 0, 'f'},
        {"site", required_argument, 0, 's'},
        {"nsfw", no_argument, 0, 1},
        {"domain", required_argument, 0, 'd'},
        {"source", required_argument, 0, 'b'},
        {"limit", required_argument, 0, 'l'},
        {"preset", required_argument, 0, 2},
        {"tor", no_argument, 0, 3},
        {"proxy", required_argument, 0, 4},
        {"timeout", required_argument, 0, 't'},
        {"threads", required_argument, 0, 'T'},
        {"output", required_argument, 0, 'o'},
        {"format", required_argument, 0, 5},
        {"engine", required_argument, 0, 9},
        {"intel", no_argument, 0, 10},
        {"about", no_argument, 0, 11},
        {"explain", required_argument, 0, 12},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 7},
        {"list-sites", no_argument, 0, 8},
        {"list-sources", no_argument, 0, 13},
        {0, 0, 0, 0}
    };

    memset(&cfg, 0, sizeof(cfg));
    memset(&store, 0, sizeof(store));
    memset(&correlation, 0, sizeof(correlation));
    memset(&harvest_summary, 0, sizeof(harvest_summary));
    cfg.timeout_ms = DEFAULT_TIMEOUT_MS;
    cfg.thread_count = 20;
    snprintf(cfg.output_format, sizeof(cfg.output_format), "txt");
    snprintf(cfg.engine, sizeof(cfg.engine), "auto");
    snprintf(cfg.preset, sizeof(cfg.preset), "balanced");

    while ((opt = getopt_long(argc, argv, "u:fs:d:b:l:t:T:o:vh", long_opts, &idx)) != -1) {
        switch (opt) {
            case 'u': add_target(&cfg, optarg); do_scan = true; break;
            case 'f': cfg.fast_scan = true; break;
            case 's': snprintf(cfg.site_filter, sizeof(cfg.site_filter), "%s", optarg); break;
            case 1: cfg.nsfw = true; break;
            case 'd': snprintf(domain, sizeof(domain), "%s", optarg); do_harvest = true; break;
            case 'b': snprintf(source, sizeof(source), "%s", optarg); break;
            case 'l': limit = atoi(optarg); break;
            case 2: snprintf(cfg.preset, sizeof(cfg.preset), "%s", optarg); break;
            case 3:
                snprintf(cfg.proxy, sizeof(cfg.proxy), "socks5://127.0.0.1:9050");
                cfg.tor_enabled = true;
                break;
            case 4: snprintf(cfg.proxy, sizeof(cfg.proxy), "%s", optarg); break;
            case 't': cfg.timeout_ms = atoi(optarg); break;
            case 'T': cfg.thread_count = atoi(optarg); break;
            case 'o': snprintf(cfg.output_file, sizeof(cfg.output_file), "%s", optarg); break;
            case 5: snprintf(cfg.output_format, sizeof(cfg.output_format), "%s", optarg); break;
            case 9: snprintf(cfg.engine, sizeof(cfg.engine), "%s", optarg); break;
            case 10: cfg.mod_intelligence = true; break;
            case 11: show_about = true; break;
            case 12: snprintf(explain_topic, sizeof(explain_topic), "%s", optarg); break;
            case 'v': verbosity++; break;
            case 'h':
                banner_print();
                help_menu_print(argv[0]);
                return 0;
            case 7:
                printf("%s %s\n", meta->name, meta->version);
                return 0;
            case 8: list_sites = true; break;
            case 13: list_sources = true; break;
            default:
                help_menu_print(argv[0]);
                return 1;
        }
    }

    while (optind < argc) {
        add_target(&cfg, argv[optind++]);
        do_scan = true;
    }

    cfg.verbose = verbosity >= 1;
    cfg.very_verbose = verbosity >= 2;
    if (cfg.thread_count < 1) cfg.thread_count = 1;
    if (cfg.timeout_ms < 1000) cfg.timeout_ms = 1000;
    apply_preset(&cfg, cfg.preset);

    if (strcasecmp(cfg.preset, "beginner") != 0 &&
        strcasecmp(cfg.preset, "balanced") != 0 &&
        strcasecmp(cfg.preset, "aggressive") != 0) {
        fprintf(stderr, "%s[ERROR]%s Unknown preset: %s\n", RED, RESET, cfg.preset);
        help_menu_print(argv[0]);
        return 1;
    }

    if (show_about) {
        about_print();
        return 0;
    }
    if (explain_topic[0]) {
        banner_print();
        if (!help_menu_print_topic(argv[0], explain_topic)) {
            fprintf(stderr, "%s[ERROR]%s Unknown explain topic: %s\n", RED, RESET, explain_topic);
            help_menu_print(argv[0]);
            return 1;
        }
        return 0;
    }

    banner_print();

    if (!do_scan && !do_harvest && !list_sites && !list_sources) {
        help_menu_print(argv[0]);
        return 0;
    }

    if (cfg.thread_count > 50 && (do_scan || do_harvest)) {
        resource_guard_confirm_threads(cfg.thread_count);
    }

    http_global_init();
    if (do_scan || list_sites) {
        cfg.sites = config_load_sites("platforms", &cfg.site_count);
    }

    if (cfg.fast_scan && cfg.site_count > 50) cfg.site_count = 50;
    if ((do_scan || list_sites) && (!cfg.sites || cfg.site_count == 0)) {
        fprintf(stderr, "%s[ERROR]%s Could not load site database.\n", RED, RESET);
        http_global_cleanup();
        free(cfg.sites);
        return 1;
    }

    if (list_sites) {
        output_list_sites(cfg.sites, cfg.site_count);
    } else if (list_sources) {
        harvester_print_sources();
    } else if (do_harvest && !do_scan) {
        harvester_run(domain, source, limit, &cfg, &harvest_summary);
        report_bundle_write_harvest(&harvest_summary, &cfg);
        harvester_summary_free(&harvest_summary);
    } else if (do_scan && !do_harvest && cfg.target_count > 0) {
        pthread_mutex_init(&store.lock, NULL);
        scanner_run(&cfg, &store);
        brief_report_process(&store, &cfg);
        if (cfg.mod_intelligence || strcasecmp(cfg.engine, "intelligence") == 0) {
            intelligence_process(&store);
        }
        output_results(&store, &cfg);
        report_bundle_write_scan(&store, &cfg);
        pthread_mutex_destroy(&store.lock);
        free(store.results);
    } else if (do_scan && do_harvest && cfg.target_count > 0) {
        LowHuntConfig runtime_cfg = cfg;
        runtime_cfg.output_file[0] = '\0';
        pthread_mutex_init(&store.lock, NULL);
        harvester_run(domain, source, limit, &runtime_cfg, &harvest_summary);
        scanner_run(&cfg, &store);
        brief_report_process(&store, &cfg);
        if (cfg.mod_intelligence || strcasecmp(cfg.engine, "intelligence") == 0) {
            intelligence_process(&store);
        }
        output_results(&store, &runtime_cfg);
        correlation_analyze(&store, &harvest_summary, &cfg, &correlation);
        correlation_print(&correlation, &cfg);
        output_investigation(&store, &harvest_summary, &correlation, &cfg);
        report_bundle_write_scan(&store, &cfg);
        report_bundle_write_harvest(&harvest_summary, &cfg);
        report_bundle_write_investigation(&store, &harvest_summary, &correlation, &cfg);
        harvester_summary_free(&harvest_summary);
        pthread_mutex_destroy(&store.lock);
        free(store.results);
    }

    free(cfg.sites);
    http_global_cleanup();
    return 0;
}
