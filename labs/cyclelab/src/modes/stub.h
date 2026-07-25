#ifndef CYCLELAB_MODE_STUB_H
#define CYCLELAB_MODE_STUB_H

/* Handles every mode BLUEPRINT.md Section 8 names that isn't implemented
 * yet. Keeping these as a recognized-but-stubbed CLI surface (rather than
 * a plain "unknown mode" error) means later chapters, scripts, and tests
 * can already reference the full command surface (Section 8: "the reader
 * should not spend each chapter learning a new codebase"). Always prints
 * "<mode>: not yet implemented" to stderr and returns 2. */
int stub_run(const char *mode);

#endif /* CYCLELAB_MODE_STUB_H */
