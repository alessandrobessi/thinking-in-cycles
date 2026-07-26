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

typedef enum {
    CYCLELAB_PATTERN_SORTED,   /* branch mode; also aliased "sequential" for memory modes */
    CYCLELAB_PATTERN_RANDOM
} cyclelab_pattern_t;

typedef enum {
    CYCLELAB_PADDING_PACKED,
    CYCLELAB_PADDING_PADDED
} cyclelab_padding_t;

#define CYCLELAB_MAX_CHAINS 16
#define CYCLELAB_CACHE_LINE_BYTES 64

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
    int chains;                /* compute-specific, default 1, max CYCLELAB_MAX_CHAINS:
                                 * number of independent accumulator chains per thread,
                                 * for Chapter 8's dependency-chain-vs-ILP lab */
    cyclelab_pattern_t pattern; /* branch/memory-specific, default SORTED (="sequential") */
    long branch_table_size;    /* branch-specific, default 1000000: size of the
                                 * per-thread lookup table the branch mode walks */
    long working_set_bytes;    /* sequential-memory/random-memory/bandwidth-specific,
                                 * default 1048576 (1 MiB): per-thread buffer size */
    long mem_stride_slots;     /* sequential-memory-specific, default 1: how many
                                 * CYCLELAB_CACHE_LINE_BYTES-sized slots to advance
                                 * per step */
    cyclelab_padding_t padding; /* false-sharing-specific, default PACKED */
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
