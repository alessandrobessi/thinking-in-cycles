#ifndef CYCLELAB_MODE_MEMORY_H
#define CYCLELAB_MODE_MEMORY_H

#include "../cli.h"
#include "../sysinfo.h"

/* Implements both "sequential-memory" and "random-memory" modes (they
 * share this one implementation; the mode name only picks the default
 * --pattern, which --pattern itself can still override).
 *
 * Each of opts->threads worker threads builds its own --working-set-size
 * buffer, divided into CYCLELAB_CACHE_LINE_BYTES-sized slots, then
 * repeatedly chases a pointer through it: buf[cur].next, one dependent
 * load per step, so elapsed time per step is a genuine memory-latency
 * measurement, not something the compiler or CPU can hide behind
 * out-of-order execution or prefetching the way independent loads could
 * be. In --pattern=sequential (the "sequential-memory" default), the
 * chase advances --stride slots at a time; in --pattern=random (the
 * "random-memory" default), the chase follows a single-cycle random
 * permutation of every slot (built with Sattolo's algorithm, which
 * guarantees full coverage with no premature repeats -- an ordinary
 * shuffle can produce multiple short, independent cycles that would
 * understate the true working set being exercised).
 *
 * Returns a process exit code: 0 on success, 1 on an internal error. */
int memory_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host);

#endif /* CYCLELAB_MODE_MEMORY_H */
