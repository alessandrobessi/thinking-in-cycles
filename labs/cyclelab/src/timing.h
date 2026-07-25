#ifndef CYCLELAB_TIMING_H
#define CYCLELAB_TIMING_H

/* Monotonic wall-clock seconds, suitable for measuring elapsed duration.
 * Never affected by system clock adjustments. */
double timing_now_seconds(void);

#endif /* CYCLELAB_TIMING_H */
