#include <pthread.h>
#include <string.h>
#include "output.h"

int main(void) {
    ResultStore store;
    LowHuntConfig cfg;
    memset(&store, 0, sizeof(store));
    memset(&cfg, 0, sizeof(cfg));
    pthread_mutex_init(&store.lock, 0);
    output_results(&store, &cfg);
    pthread_mutex_destroy(&store.lock);
    return 0;
}
