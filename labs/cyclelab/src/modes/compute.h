#ifndef CYCLELAB_MODE_COMPUTE_H
#define CYCLELAB_MODE_COMPUTE_H

#include "../cli.h"
#include "../sysinfo.h"

/* Runs the "compute" mode: opts->threads worker threads each execute a
 * tight, data-dependent int/float accumulator chain for opts->duration_s
 * (or exactly opts->iterations iterations if set), then the run's
 * configuration, host info, and per-thread + combined checksums are
 * written to opts->output_path in opts->format.
 *
 * Returns a process exit code: 0 on success, 1 on an internal error
 * (e.g. the output file couldn't be opened, or a thread failed to start). */
int compute_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host);

#endif /* CYCLELAB_MODE_COMPUTE_H */
