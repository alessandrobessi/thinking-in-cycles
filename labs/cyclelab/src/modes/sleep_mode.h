#ifndef CYCLELAB_MODE_SLEEP_H
#define CYCLELAB_MODE_SLEEP_H

#include "../cli.h"
#include "../sysinfo.h"

/* Each of opts->threads worker threads repeatedly calls nanosleep() for
 * approximately opts->sleep_us microseconds, then does a small amount of
 * on-CPU work (a counter increment) before sleeping again. Every
 * intentional sleep is a voluntary yield of the CPU: this mode exists to
 * give Chapter 29 a clean, portable counter-example to Chapter 21-22's
 * involuntary-context-switch-dominated workloads (compute, lock-contention
 * under heavy load) -- nanosleep should drive voluntary context switches
 * up while leaving involuntary switches near zero, the opposite signature.
 *
 * Returns a process exit code: 0 on success, 1 on an internal error. */
int sleep_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host);

#endif /* CYCLELAB_MODE_SLEEP_H */
