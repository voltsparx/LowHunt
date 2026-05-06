#include <stdio.h>

#include "banner.h"
#include "colors.h"
#include "metadata.h"

void banner_print(void) {
    const LowHuntMetadata* meta = lowhunt_metadata();

    printf("%s", CYAN);
    printf("        _____  _  _  _ _     _ _     _ __   _ _______\n");
    printf("|      |     | |  |  | |_____| |     | | \\  |    |\n");
    printf("|_____ |_____| |__|__| |     | |_____| |  \\_|    |\n");
    printf("%s", RESET);
    printf("%s  %s %s%s\n", DIM, meta->name, meta->version, RESET);
    printf("%s  %s%s\n\n", BLUE, meta->tagline, RESET);
}
