#include <string.h>
#include "lowhunt.h"

int main(void) {
    LowHuntConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.thread_count = 1;
    return cfg.thread_count == 1 ? 0 : 1;
}
