#ifndef CYCLELAB_MODE_FALSE_SHARING_H
#define CYCLELAB_MODE_FALSE_SHARING_H

#include "../cli.h"
#include "../sysinfo.h"

/* Each of opts->threads worker threads repeatedly increments its own
 * dedicated counter, as fast as possible, with no lock and no reason
 * -- in source-code terms -- to interact with any other thread at all.
 * In --padding=packed (default), the counters sit in one tightly packed
 * array, so several of them typically share a CYCLELAB_CACHE_LINE_BYTES
 * cache line; in --padding=padded, each counter is padded out to its
 * own exclusive cache line. Any scaling difference between the two
 * layouts, at the same thread count, is false sharing: cache-coherence
 * traffic caused by threads writing to the same cache line, even though
 * each one only ever touches its own logically distinct counter.
 *
 * Returns a process exit code: 0 on success, 1 on an internal error. */
int false_sharing_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host);

#endif /* CYCLELAB_MODE_FALSE_SHARING_H */
