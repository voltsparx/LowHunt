#include <ctype.h>
#include <stddef.h>

void lowhunt_trim_in_place(char* s) {
    char* start = s;
    char* end;

    if (!s) return;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) {
        char* dst = s;
        while ((*dst++ = *start++) != '\0') {}
    }

    end = s;
    while (*end) end++;
    while (end > s && isspace((unsigned char)*(end - 1))) end--;
    *end = '\0';
}
