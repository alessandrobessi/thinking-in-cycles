#ifndef CYCLELAB_MODE_BRANCH_H
#define CYCLELAB_MODE_BRANCH_H

#include "../cli.h"
#include "../sysinfo.h"

/* Runs the "branch" mode: each of opts->threads worker threads builds its
 * own --branch-table-size table of pseudo-random byte values (0-255), in
 * either --pattern=sorted or --pattern=random order, then repeatedly walks
 * the table applying a data-dependent conditional (BLUEPRINT.md Chapter 9's
 * "sorted versus random data through the same conditional" worked example).
 * Sorted order groups long runs of the same branch outcome together
 * (predictable); random order does not.
 *
 * Returns a process exit code: 0 on success, 1 on an internal error. */
int branch_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host);

#endif /* CYCLELAB_MODE_BRANCH_H */
