#ifndef CYCLELAB_RUSAGE_UTIL_H
#define CYCLELAB_RUSAGE_UTIL_H

/* Process-wide voluntary/involuntary context-switch counts, via the
 * POSIX getrusage(2) RUSAGE_SELF fields ru_nvcsw/ru_nivcsw -- available
 * on both Linux and macOS (unlike RUSAGE_THREAD, which is Linux-only),
 * which is exactly why every cyclelab mode reports these process-wide
 * rather than per-thread. A voluntary switch is a thread giving up the
 * CPU on its own (blocking on I/O, a lock, a sleep); an involuntary
 * switch is the scheduler preempting a still-runnable thread to let
 * something else run -- the signal Chapters 21-22 need to make
 * "runnable pressure" and scheduling interference directly measurable
 * without perf sched or pidstat. */
void rusage_get_context_switches(long *voluntary, long *involuntary);

#endif /* CYCLELAB_RUSAGE_UTIL_H */
