#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "banner.h"
#include "colors.h"
#include "help_menu.h"
#include "metadata.h"

typedef struct {
    const char* topic;
    const char* summary;
    const char* usage;
    const char* details;
} HelpTopic;

static const HelpTopic g_topics[] = {
    {"scan", "Search usernames across the loaded platform set.",
     "lowhunt alice bob --engine fusion -vv",
     "Use positional usernames or repeat -u. Add -vv for structured terminal output of found, unknown, and failed checks."},
    {"preset", "Apply curated defaults for speed, clarity, or steadiness.",
     "lowhunt --preset beginner alice",
     "beginner favors readable output and investigator summaries. balanced keeps the default feel. aggressive pushes faster concurrency and full visibility."},
    {"harvest", "Harvest public hostnames for a domain from passive sources.",
     "lowhunt -d example.com -b crtsh -l 200",
     "Harvest mode is for public certificate, contact-page, and same-domain email discovery from public pages only. Built-in filters suppress noisy addresses like noreply, asset references, and off-domain hits."},
    {"sources", "List available passive harvest sources.",
     "lowhunt --list-sources",
     "Shows the currently wired public harvest sources. 'all' now fuses crtsh, rapiddns, and wayback to raise hostname confidence when results overlap."},
    {"engine", "Choose how LowHunt executes requests.",
     "lowhunt --engine auto|threadpool|fusion|async|parallel|stabilizer|sync USERNAME",
     "auto picks a strategy from workload size and network posture. stabilizer favors steadier pacing. fusion and parallel push for speed."},
    {"fusion", "High-throughput engine for large username batches.",
     "lowhunt --engine fusion alice bob charlie",
     "fusion raises concurrency above the requested thread count for fast scans when your host and network can handle it."},
    {"stabilizer", "Lower-pressure engine for proxies, Tor, or fragile targets.",
     "lowhunt --engine stabilizer --tor alice",
     "stabilizer runs sequentially with pauses so the tool stays usable under rate pressure and less likely to trip remote throttles."},
    {"threads", "Set the requested concurrency budget.",
     "lowhunt --threads 120 alice",
     "There is no hard top cap now. Above 50, LowHunt checks local resources and warns before continuing if the machine looks constrained."},
    {"-v", "Enable verbose status output.",
     "lowhunt -v alice",
     "Verbose mode keeps the normal found-focused output but shows more run context and engine choice."},
    {"-vv", "Enable full verbose structured output.",
     "lowhunt -vv alice",
     "Full verbose mode prints found, not-found, unknown, and error results in a more complete terminal view for investigators."},
    {"about", "Show project information, banner, safety, and repository details.",
     "lowhunt --about",
     "about prints the banner first and then a quick project overview, contact details, repository URL, and scope reminder."},
    {"reports", "Review the stored report bundle after a run.",
     "lowhunt alice && cat ~/.lowhunt/output/reports/batch/<timestamp>/report.txt",
     "Every scan and harvest now stores a small offline bundle with CLI, TXT, and JSON artifacts under ~/.lowhunt/output/reports/."},
    {"manual", "Read the installed manual page.",
     "man lowhunt",
     "The manual page is installed into the local man path by the managed installer scripts in install/."}
};

static const HelpTopic* find_topic(const char* raw) {
    size_t count = sizeof(g_topics) / sizeof(g_topics[0]);
    if (!raw || !raw[0]) return NULL;

    for (size_t i = 0; i < count; i++) {
        if (strcasecmp(raw, g_topics[i].topic) == 0) return &g_topics[i];
    }
    return NULL;
}

void help_menu_print(const char* prog) {
    const LowHuntMetadata* meta = lowhunt_metadata();

    printf("%s%s Help%s\n\n", BOLD, meta->name, RESET);
    printf("Usage:\n");
    printf("  %s [OPTIONS] <username> [username2 ...]\n", prog);
    printf("  %s -d <domain> [OPTIONS]\n\n", prog);

    printf("Core:\n");
    printf("  -u, --username <name>  Add a username target\n");
    printf("  -d, --domain <domain>  Run passive hostname harvest mode\n");
    printf("  -b, --source <source>  Harvest source: crtsh or all\n");
    printf("  -l, --limit <n>        Limit harvested hostnames\n");
    printf("  -f, --fast             Trim the loaded platform set to the fast subset\n");
    printf("  -s, --site <name>      Limit scanning to one platform\n");
    printf("  --preset <name>        beginner, balanced, or aggressive\n");
    printf("  --engine <name>        auto, threadpool, fusion, async, parallel, stabilizer, sync\n");
    printf("  -t, --timeout <ms>     Per-request timeout in milliseconds\n");
    printf("  -T, --threads <n>      Requested concurrency budget\n");
    printf("  --tor                  Route traffic through socks5://127.0.0.1:9050\n");
    printf("  --proxy <url>          Use a custom proxy\n");
    printf("  --intel                Show the investigator summary engine\n");
    printf("  -v                     Verbose output\n");
    printf("  -vv                    Full verbose structured output\n");
    printf("  -o, --output <file>    Save results to a file\n");
    printf("  --format <fmt>         txt, json, or csv\n");
    printf("  --nsfw                 Include NSFW platforms\n");
    printf("  --list-sites           List supported platforms\n");
    printf("  --list-sources         List available passive harvest sources\n");
    printf("  --about                Show banner and project details\n");
    printf("  --explain <topic>      Explain a command or concept\n");
    printf("  --version              Print the current version\n");
    printf("  -h, --help             Show this help menu\n\n");

    printf("Examples:\n");
    printf("  %s alice --preset beginner\n", prog);
    printf("  %s alice --engine fusion -vv\n", prog);
    printf("  %s -u alice -u bob --threads 120 --intel\n", prog);
    printf("  %s -d example.com -b crtsh -o hosts.json --format json\n", prog);
    printf("  %s --list-sources\n", prog);
    printf("  %s --about\n", prog);
    printf("  %s --explain engine\n\n", prog);

    printf("Topics:\n");
    printf("  scan, preset, harvest, sources, engine, fusion, stabilizer, threads, -v, -vv, about, reports, manual\n\n");
    printf("%sAuthorized public-data OSINT only.%s\n", YELLOW, RESET);
}

bool help_menu_print_topic(const char* prog, const char* topic) {
    const HelpTopic* entry = find_topic(topic);

    if (!entry) return false;
    printf("%sExplain: %s%s\n\n", BOLD, entry->topic, RESET);
    printf("%sSummary:%s %s\n\n", BLUE, RESET, entry->summary);
    printf("%sUsage:%s\n  %s\n\n", BLUE, RESET, entry->usage);
    printf("%sDetails:%s %s\n\n", BLUE, RESET, entry->details);
    printf("Try: %s --help\n", prog);
    return true;
}

void about_print(void) {
    const LowHuntMetadata* meta = lowhunt_metadata();

    banner_print();
    printf("%sAbout%s\n", BOLD, RESET);
    printf("  Name: %s\n", meta->name);
    printf("  Version: %s\n", meta->version);
    printf("  Author: %s\n", meta->author);
    printf("  Contact: %s\n", meta->email);
    printf("  Repository: %s\n", meta->repo_url);
    printf("  License: %s\n", meta->license_name);
    printf("  Manual: man %s\n", meta->manual_name);
    printf("\n");
    printf("%s%s%s\n", BLUE, meta->tagline, RESET);
    printf("Built for public, authorized OSINT on usernames and domains.\n");
    printf("It checks public platform footprints, passive hostnames, and investigator-friendly summaries.\n");
    printf("\n");
    printf("%sUse only on targets you own or have written authorization to assess.%s\n",
           YELLOW, RESET);
}
