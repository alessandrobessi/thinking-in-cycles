#ifndef CYCLELAB_SYSINFO_H
#define CYCLELAB_SYSINFO_H

typedef struct {
    char os[64];
    char kernel[160];
    char arch[32];
    char hostname[256];
    int logical_cpus;
} cyclelab_hostinfo_t;

/* Collects host information via uname(2)/sysconf(3). Every field is
 * always populated with *something* -- "unknown" on failure, never left
 * blank -- so JSON output never has to special-case a missing host block
 * (BLUEPRINT.md Section 8: "gracefully reports unavailable features"). */
void sysinfo_collect(cyclelab_hostinfo_t *info);

#endif /* CYCLELAB_SYSINFO_H */
