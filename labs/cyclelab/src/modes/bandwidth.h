#ifndef CYCLELAB_MODE_BANDWIDTH_H
#define CYCLELAB_MODE_BANDWIDTH_H

#include "../cli.h"
#include "../sysinfo.h"

/* Each of opts->threads worker threads repeatedly streams sequentially
 * through its own --working-set-size buffer of doubles, summing every
 * element. Unlike sequential-memory/random-memory's pointer chase
 * (deliberately dependent, to measure latency), this is a simple,
 * independent-iteration loop the compiler can vectorize and the CPU can
 * aggressively prefetch -- the point is to measure sustained bandwidth,
 * which requires letting the hardware do everything it can to hide
 * latency, not defeating it.
 *
 * Use a --working-set-size well beyond your machine's last-level cache
 * to measure real DRAM bandwidth rather than cache bandwidth.
 *
 * Returns a process exit code: 0 on success, 1 on an internal error. */
int bandwidth_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host);

#endif /* CYCLELAB_MODE_BANDWIDTH_H */
