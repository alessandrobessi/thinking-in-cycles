#ifndef CYCLELAB_SYSINFO_H
#define CYCLELAB_SYSINFO_H

typedef struct {
    char os[64];
    char kernel[160];
    char arch[32];
    char hostname[256];
    int logical_cpus;
    /* Detected L1 data cache line size in bytes, via sysconf/sysctl; 0 if
     * this platform offers no way to ask. Used by false-sharing and
     * memory modes to warn when the actual coherence granule exceeds
     * CYCLELAB_CACHE_LINE_BYTES, the compiled-in assumption those modes'
     * padding and slot-size arithmetic are built on -- see each mode's
     * source for why a *smaller* detected value is not a problem, but a
     * larger one would quietly invalidate their "no sharing possible" /
     * "one slot visited per cache line" guarantees. */
    int cache_line_bytes_detected;
} cyclelab_hostinfo_t;

/* Collects host information via uname(2)/sysconf(3). Every field is
 * always populated with *something* -- "unknown" on failure, never left
 * blank -- so JSON output never has to special-case a missing host block
 * (BLUEPRINT.md Section 8: "gracefully reports unavailable features"). */
void sysinfo_collect(cyclelab_hostinfo_t *info);

#endif /* CYCLELAB_SYSINFO_H */
