#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "colors.h"
#include "resource_guard.h"

static unsigned long long physical_memory_mb(void) {
#ifdef _WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (!GlobalMemoryStatusEx(&statex)) return 0;
    return (unsigned long long)(statex.ullTotalPhys / (1024ULL * 1024ULL));
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages <= 0 || page_size <= 0) return 0;
    return (unsigned long long)pages * (unsigned long long)page_size / (1024ULL * 1024ULL);
#endif
}

static int cpu_count(void) {
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (int)info.dwNumberOfProcessors;
#else
    long cpus = sysconf(_SC_NPROCESSORS_ONLN);
    return cpus > 0 ? (int)cpus : 0;
#endif
}

bool resource_guard_confirm_threads(int requested_threads) {
    unsigned long long mem_mb;
    int cpus;
    bool looks_low;

    if (requested_threads <= 50) return true;

    mem_mb = physical_memory_mb();
    cpus = cpu_count();
    looks_low = (cpus > 0 && cpus <= 2) || (mem_mb > 0 && mem_mb < 4096ULL);

    printf("%s[INFO]%s Requested threads: %d\n", BLUE, RESET, requested_threads);
    if (cpus > 0) printf("%s[INFO]%s Detected CPU cores: %d\n", BLUE, RESET, cpus);
    if (mem_mb > 0) printf("%s[INFO]%s Detected physical memory: %llu MB\n", BLUE, RESET, mem_mb);

    if (!looks_low) return true;

    printf("%s[WARN]%s This machine looks resource-constrained for very high concurrency.\n",
           YELLOW, RESET);
    printf("Press %sEnter%s to continue anyway, or press %sCtrl+C%s to cancel.\n",
           BOLD, RESET, BOLD, RESET);
    (void)getchar();
    return true;
}
