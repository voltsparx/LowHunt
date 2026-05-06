#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#define LOWHUNT_VERSION     "1.0.0"
#define LOWHUNT_AUTHOR      "voltsparx"
#define LOWHUNT_EMAIL       "voltsparx@gmail.com"
#define MAX_TARGETS         64
#define MAX_SITES           2048
#define MAX_THREADS         50
#define DEFAULT_TIMEOUT_MS  10000
#define DEFAULT_USER_AGENT  "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36"

typedef enum {
    RESULT_FOUND = 0,
    RESULT_NOT_FOUND = 1,
    RESULT_UNKNOWN = 2,
    RESULT_ERROR = 3
} ResultStatus;

typedef enum {
    DETECT_STATUS_CODE = 0,
    DETECT_STRING_MATCH = 1,
    DETECT_RESPONSE_URL = 2
} DetectMethod;

typedef struct {
    char name[128];
    char url_template[512];
    char error_msg[256];
    char error_url[512];
    int error_code;
    DetectMethod method;
    bool is_nsfw;
    char category[64];
} Site;

typedef struct {
    char username[128];
    char url[512];
    ResultStatus status;
    char site_name[128];
    long http_code;
    double response_time_ms;
} ScanResult;

typedef struct {
    Site* sites;
    int site_count;
    char targets[MAX_TARGETS][128];
    int target_count;
    bool fast_scan;
    bool tor_enabled;
    bool verbose;
    bool no_color;
    bool print_all;
    int timeout_ms;
    int thread_count;
    char proxy[256];
    char output_file[512];
    char output_format[16];
    char site_filter[128];
    bool nsfw;
} LowHuntConfig;

typedef struct {
    ScanResult* results;
    int count;
    int capacity;
    pthread_mutex_t lock;
} ResultStore;
