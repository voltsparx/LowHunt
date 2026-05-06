#pragma once

#include <stdbool.h>
#include "lowhunt.h"

typedef enum {
    MOD_TOR = 0,
    MOD_DARKWEB = 1,
    MOD_SCRAPER = 2,
    MOD_EMAIL = 3,
    MOD_PHONE = 4,
    MOD_DNS = 5,
    MOD_WHOIS = 6,
    MOD_PERMUTE = 7
} ModuleType;

typedef struct Module Module;

struct Module {
    ModuleType type;
    const char* name;
    const char* description;
    const char* version;
    bool requires_tor;
    bool requires_root;
    bool (*init)(Module* self, const LowHuntConfig* cfg);
    void (*run)(Module* self, const LowHuntConfig* cfg, ResultStore* store);
    void (*destroy)(Module* self);
    void (*print_results)(Module* self, bool no_color);
    void* priv;
};

#define MAX_MODULES 16
typedef struct {
    Module* mods[MAX_MODULES];
    int count;
} ModuleRegistry;

ModuleRegistry* module_registry_create(void);
bool module_register(ModuleRegistry* reg, Module* mod);
Module* module_get(ModuleRegistry* reg, ModuleType type);
void module_registry_destroy(ModuleRegistry* reg);
Module* module_create(ModuleType type);
void module_free(Module* m);

Module* mod_tor_create(void);
Module* mod_darkweb_create(void);
Module* mod_scraper_create(void);
Module* mod_email_create(void);
Module* mod_phone_create(void);
Module* mod_dns_create(void);
Module* mod_whois_create(void);
Module* mod_permute_create(void);

const char* mod_tor_proxy_url(Module* self);
int mod_permute_get_list(Module* self, char out[][128], int max);
