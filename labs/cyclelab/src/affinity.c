#define _GNU_SOURCE
#include "affinity.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__linux__)
#include <sched.h>

static int cpu_list_from_spec(const char *spec, int *ids, int max_ids) {
    /* Parses "2,3,7" into ids[]; returns count, or 0 if spec has no digits. */
    int count = 0;
    char buf[256];
    strncpy(buf, spec, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *save = NULL;
    char *tok = strtok_r(buf, ",", &save);
    while (tok != NULL && count < max_ids) {
        char *end = NULL;
        long v = strtol(tok, &end, 10);
        if (end != tok && v >= 0) {
            ids[count++] = (int)v;
        }
        tok = strtok_r(NULL, ",", &save);
    }
    return count;
}
#endif /* __linux__ */

cyclelab_affinity_result_t affinity_apply(const char *spec, int thread_index,
                                           int nthreads, const char **reason_out) {
    (void)nthreads;
    *reason_out = NULL;

    if (spec == NULL || strcmp(spec, "none") == 0) {
        return CYCLELAB_AFFINITY_SKIPPED;
    }

#if defined(__linux__)
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpu <= 0) ncpu = 1;

    int target_cpu;
    if (strcmp(spec, "spread") == 0) {
        target_cpu = thread_index % (int)ncpu;
    } else {
        int ids[64];
        int n = cpu_list_from_spec(spec, ids, 64);
        if (n == 0) {
            *reason_out = "affinity spec could not be parsed; running without pinning";
            return CYCLELAB_AFFINITY_UNSUPPORTED;
        }
        target_cpu = ids[thread_index % n];
    }

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(target_cpu, &set);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
    if (rc != 0) {
        *reason_out = "pthread_setaffinity_np failed (insufficient permissions or invalid CPU id); running without pinning";
        return CYCLELAB_AFFINITY_UNSUPPORTED;
    }
    return CYCLELAB_AFFINITY_APPLIED;
#else
    /* macOS and other non-Linux targets: there is no portable, unprivileged
     * hard-affinity API. Report the limitation instead of silently
     * pretending to pin (BLUEPRINT.md Section 8). */
    (void)thread_index;
    *reason_out = "CPU affinity pinning is not supported on this OS; running without pinning";
    return CYCLELAB_AFFINITY_UNSUPPORTED;
#endif
}
