#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "colors.h"
#include "correlation.h"

static void lowercase_copy(char* out, size_t out_size, const char* in) {
    size_t i = 0;

    if (!out || out_size == 0) return;
    while (in && in[i] && i + 1 < out_size) {
        out[i] = (char)tolower((unsigned char)in[i]);
        i++;
    }
    out[i] = '\0';
}

static void normalize_handle(char* out, size_t out_size, const char* in) {
    size_t i = 0;

    if (!out || out_size == 0) return;
    while (in && *in && i + 1 < out_size) {
        unsigned char c = (unsigned char)*in++;
        if (isalnum(c)) {
            out[i++] = (char)tolower(c);
        }
    }
    out[i] = '\0';
}

static bool localpart_matches_username(const char* email, const char* username, bool exact) {
    const char* at;
    char local[128];
    char normalized_local[128];
    char normalized_user[128];

    if (!email || !username) return false;
    at = strchr(email, '@');
    if (!at) return false;
    if ((size_t)(at - email) >= sizeof(local)) return false;

    memcpy(local, email, (size_t)(at - email));
    local[at - email] = '\0';
    normalize_handle(normalized_local, sizeof(normalized_local), local);
    normalize_handle(normalized_user, sizeof(normalized_user), username);

    if (!normalized_local[0] || !normalized_user[0]) return false;
    if (exact) return strcmp(normalized_local, normalized_user) == 0;
    return strstr(normalized_local, normalized_user) != NULL ||
           strstr(normalized_user, normalized_local) != NULL;
}

void correlation_analyze(const ResultStore* store, const HarvestSummary* harvest,
                         const LowHuntConfig* cfg, CorrelationSummary* out) {
    int found_profiles = 0;
    int exact_matches = 0;
    int partial_matches = 0;
    int correlated = 0;
    int confidence = 20;

    (void)cfg;
    if (!out) return;
    memset(out, 0, sizeof(*out));

    if (store) {
        for (int i = 0; i < store->count; i++) {
            if (store->results[i].status == RESULT_FOUND) found_profiles++;
        }
    }

    if (store && harvest) {
        for (int t = 0; t < cfg->target_count; t++) {
            bool matched = false;
            for (int e = 0; e < harvest->email_count; e++) {
                if (localpart_matches_username(harvest->emails[e].email, cfg->targets[t], true)) {
                    exact_matches++;
                    matched = true;
                } else if (localpart_matches_username(harvest->emails[e].email, cfg->targets[t], false)) {
                    partial_matches++;
                    matched = true;
                }
            }
            if (matched) {
                correlated++;
            }
        }
    }

    confidence += found_profiles > 0 ? 20 : 0;
    confidence += correlated * 15;
    confidence += exact_matches * 10;
    confidence += partial_matches * 5;
    if (harvest && harvest->host_count >= 5) confidence += 10;
    if (harvest && harvest->email_count > 0) confidence += 10;
    if (confidence > 95) confidence = 95;

    out->found_profiles = found_profiles;
    out->exact_email_matches = exact_matches;
    out->partial_email_matches = partial_matches;
    out->correlated_usernames = correlated;
    out->host_count = harvest ? harvest->host_count : 0;
    out->email_count = harvest ? harvest->email_count : 0;
    out->confidence_score = confidence;

    if (correlated > 0 && exact_matches > 0) {
        snprintf(out->narrative, sizeof(out->narrative),
                 "Strong cross-signal overlap: public profile hits and same-domain contact identifiers align with at least one supplied username.");
    } else if (correlated > 0 || partial_matches > 0) {
        snprintf(out->narrative, sizeof(out->narrative),
                 "Moderate cross-signal overlap: some public identifiers appear related across profile and domain-side artifacts.");
    } else if (found_profiles > 0 && harvest && harvest->email_count > 0) {
        snprintf(out->narrative, sizeof(out->narrative),
                 "Parallel signals detected: public profiles and domain-side contacts were both observed, but no direct username-to-email correlation was confirmed.");
    } else if (harvest && (harvest->host_count > 0 || harvest->email_count > 0)) {
        snprintf(out->narrative, sizeof(out->narrative),
                 "Domain-side intelligence was collected, but the supplied usernames were not directly correlated to harvested contact identifiers.");
    } else {
        snprintf(out->narrative, sizeof(out->narrative),
                 "No meaningful cross-signal intelligence overlap was confirmed in this run.");
    }
}

void correlation_print(const CorrelationSummary* summary, const LowHuntConfig* cfg) {
    (void)cfg;
    if (!summary) return;

    printf("\n%s[correlation]%s Investigation fusion\n", CYAN, RESET);
    printf("Confidence score: %d\n", summary->confidence_score);
    printf("Found profiles: %d\n", summary->found_profiles);
    printf("Harvested hosts: %d\n", summary->host_count);
    printf("Harvested emails: %d\n", summary->email_count);
    if (summary->exact_email_matches > 0 || summary->partial_email_matches > 0) {
        printf("Exact username/email matches: %d\n", summary->exact_email_matches);
        printf("Loose username/email matches: %d\n", summary->partial_email_matches);
        printf("Correlated usernames: %d\n", summary->correlated_usernames);
    }
    printf("%s%s%s\n", BLUE, summary->narrative, RESET);
}
