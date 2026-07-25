#ifndef CYCLELAB_AFFINITY_H
#define CYCLELAB_AFFINITY_H

typedef enum {
    CYCLELAB_AFFINITY_SKIPPED,   /* spec was "none": no attempt made, no warning */
    CYCLELAB_AFFINITY_APPLIED,   /* pinning succeeded */
    CYCLELAB_AFFINITY_UNSUPPORTED /* pinning was requested but this OS/spec can't do it */
} cyclelab_affinity_result_t;

/* Best-effort attempt to pin the calling thread (thread_index of nthreads
 * total) according to spec ("none" | "spread" | comma-separated CPU list).
 * Never fatal: on unsupported platforms or invalid specs this reports
 * CYCLELAB_AFFINITY_UNSUPPORTED and writes a human-readable reason into
 * *reason_out (a static string owned by the callee, not to be freed) --
 * BLUEPRINT.md Section 8's "gracefully reports unavailable features". */
cyclelab_affinity_result_t affinity_apply(const char *spec, int thread_index,
                                           int nthreads, const char **reason_out);

#endif /* CYCLELAB_AFFINITY_H */
