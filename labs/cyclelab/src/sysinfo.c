#include "sysinfo.h"

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#if defined(__x86_64__) || defined(_M_X64)
#define CYCLELAB_ARCH "x86_64"
#elif defined(__aarch64__) || defined(_M_ARM64)
#define CYCLELAB_ARCH "arm64"
#else
#define CYCLELAB_ARCH "unknown"
#endif

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
}
