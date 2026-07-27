#include "sysinfo.h"

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define CYCLELAB_ARCH "x86_64"
#elif defined(__aarch64__) || defined(_M_ARM64)
#define CYCLELAB_ARCH "arm64"
#else
#define CYCLELAB_ARCH "unknown"
#endif

/* Best-effort L1 data cache line size, in bytes; 0 if this platform
 * offers no portable way to ask. Never guessed or hardcoded here -- a
 * caller that gets 0 back should treat the line size as unknown, not
 * assume any specific value. */
static int detect_cache_line_bytes(void) {
#if defined(__APPLE__)
    int64_t line = 0;
    size_t len = sizeof(line);
    if (sysctlbyname("hw.cachelinesize", &line, &len, NULL, 0) == 0 && line > 0) {
        return (int)line;
    }
    return 0;
#elif defined(_SC_LEVEL1_DCACHE_LINESIZE)
    long line = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    return (line > 0) ? (int)line : 0;
#else
    return 0;
#endif
}

static void safe_copy(char *dst, size_t dst_size, const char *src) {
    if (src == NULL || src[0] == '\0') {
        snprintf(dst, dst_size, "unknown");
        return;
    }
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

void sysinfo_collect(cyclelab_hostinfo_t *info) {
    memset(info, 0, sizeof(*info));

    struct utsname u;
    if (uname(&u) == 0) {
        safe_copy(info->os, sizeof(info->os), u.sysname);
        safe_copy(info->kernel, sizeof(info->kernel), u.release);
        safe_copy(info->hostname, sizeof(info->hostname), u.nodename);
    } else {
        safe_copy(info->os, sizeof(info->os), NULL);
        safe_copy(info->kernel, sizeof(info->kernel), NULL);
        safe_copy(info->hostname, sizeof(info->hostname), NULL);
    }

    safe_copy(info->arch, sizeof(info->arch), CYCLELAB_ARCH);

    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    info->logical_cpus = (ncpu > 0) ? (int)ncpu : 1;

    info->cache_line_bytes_detected = detect_cache_line_bytes();
}
