#ifndef CYCLELAB_CLI_H
#define CYCLELAB_CLI_H

/* Command-line surface for cyclelab. This interface is meant to stay
 * stable across the whole book (BLUEPRINT.md Section 8): new modes get
 * added, but existing flags keep meaning what they mean today. */

typedef enum {
    CYCLELAB_OP_INT,
    CYCLELAB_OP_FLOAT,
    CYCLELAB_OP_MIXED
} cyclelab_op_t;

typedef enum {
    CYCLELAB_FMT_JSON,
    CYCLELAB_FMT_TEXT
} cyclelab_format_t;

typedef struct {
    const char *mode;          /* "compute", "branch", ... */
    double duration_s;         /* default 2.0; ignored if iterations > 0 */
    long iterations;           /* 0 = unset, use duration_s instead */
    int threads;                /* default 1 */
    const char *affinity_spec; /* "none" (default) | "spread" | comma CPU list */
    unsigned long seed;        /* default derived from time+pid */
    cyclelab_format_t format;  /* default JSON */
    const char *output_path;   /* "-" (default) = stdout */
    int quiet;                 /* suppress warnings on stderr */
    cyclelab_op_t op;          /* compute-specific, default MIXED */
} cyclelab_options_t;

void cli_defaults(cyclelab_options_t *opts);
void cli_print_usage(const char *prog);
void cli_print_version(void);

/* Parses argv. On --help/--version or a usage error, prints the
 * appropriate message and sets *exit_now = 1 with *exit_code set to the
 * process's intended exit code (0 for help/version, 64 for bad usage,
 * matching BSD sysexits.h EX_USAGE). On success, *exit_now stays 0 and
 * opts->mode is set. Returns 0 on success, -1 on error (mirrors
 * *exit_now but kept as a return value for direct call-site checks). */
int cli_parse(int argc, char **argv, cyclelab_options_t *opts,
              int *exit_now, int *exit_code);

#endif /* CYCLELAB_CLI_H */
