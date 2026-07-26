#ifndef CYCLELAB_MODE_LOCK_CONTENTION_H
#define CYCLELAB_MODE_LOCK_CONTENTION_H

#include "../cli.h"
#include "../sysinfo.h"

/* Each of opts->threads worker threads repeatedly contends for one
 * shared pthread_mutex protecting a single shared counter. While
 * holding the lock, a thread busy-spins for approximately
 * opts->lock_hold_us microseconds before incrementing and releasing --
 * simulating a critical section that does real work rather than an
 * instant increment, so contention has something to wait for.
 *
 * Unlike false-sharing (same cache line, no logical dependency between
 * threads), lock-contention has a genuine serialization point: only one
 * thread can be inside the critical section at a time, no matter how
 * many CPUs are free. pthread_mutex blocks a waiting thread rather than
 * spinning it, so heavy contention should show up directly in the
 * voluntary context-switch count.
 *
 * Returns a process exit code: 0 on success, 1 on an internal error. */
int lock_contention_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host);

#endif /* CYCLELAB_MODE_LOCK_CONTENTION_H */
