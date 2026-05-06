#include "http.h"

int main(void) {
    http_global_init();
    http_global_cleanup();
    return 0;
}
