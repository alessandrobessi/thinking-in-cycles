#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static const char *KNOWN_MODES[] = {
    "compute", "branch", "sequential-memory", "random-memory", "bandwidth",
    "false-sharing", "lock-contention", "syscall", "sleep", "numa", "mixed",
    NULL
};

static int mode_is_known(const char *mode) {
    for (int i = 0; KNOWN_MODES[i] != NULL; i++) {
        if (strcmp(mode, KNOWN_MODES[i]) == 0) return 1;
    }
    return 0;
}

void cli_defaults(cyclelab_options_t *opts) {
    opts->mode = NULL;
    opts->duration_s = 2.0;
    opts->iterations = 0;
    opts->threads = 1;
    opts->affinity_spec = "none";
    opts->seed = (unsigned long)time(NULL) ^ (unsigned long)getpid();
    opts->format = CYCLELAB_FMT_JSON;
    opts->output_path = "-";
    opts->quiet = 0;
    opts->op = CYCLELAB_OP_MIXED;
    opts->chains = 1;
    opts->pattern = CYCLELAB_PATTERN_SORTED;
    opts->branch_table_size = 1000000;
    opts->working_set_bytes = 1048576; /* 1 MiB */
    opts->mem_stride_slots = 1;
    opts->padding = CYCLELAB_PADDING_PACKED;
    opts->lock_hold_us = 5.0;
    opts->sleep_us = 1000.0;
}

void cli_print_usage(const char *prog) {
    fprintf(stderr,
        "usage: %s <mode> [options]\n"
        "\n"
        "modes:\n"
        "  compute, branch, sequential-memory, random-memory, bandwidth,\n"
        "  false-sharing, lock-contention, sleep\n"
        "                        implemented\n"
        "  syscall, numa, mixed  recognized, not yet implemented\n"
        "\n"
        "global options:\n"
        "  --duration=SEC        time-boxed run length (default 2.0)\n"
        "  --iterations=N        fixed per-thread iteration count (overrides --duration)\n"
        "  --threads=N           worker thread count (default 1)\n"
        "  --affinity=SPEC       none (default) | spread | comma-separated CPU list\n"
        "  --seed=N              PRNG seed (default: time+pid derived)\n"
        "  --format=json|text    output format (default json)\n"
        "  --output=PATH|-       output destination (default \"-\", i.e. stdout)\n"
        "  --quiet               suppress warnings on stderr\n"
        "  --help                show this message and exit\n"
        "  --version             show version and exit\n"
        "\n"
        "compute-specific options:\n"
        "  --op=int|float|mixed  which accumulator chain to run (default mixed)\n"
        "  --chains=N            independent accumulator chains per thread, 1-%d (default 1)\n"
        "\n"
        "branch-specific options:\n"
        "  --pattern=sorted|random   table order to walk (default sorted)\n"
        "  --branch-table-size=N     per-thread table size (default 1000000)\n"
        "\n"
        "sequential-memory/random-memory-specific options:\n"
        "  --working-set-size=BYTES  per-thread buffer size, accepts K/M/G suffixes\n"
        "                            (default 1048576, i.e. 1M)\n"
        "  --pattern=sequential|random  overrides the mode's default access order\n"
        "  --stride=N                slots (of %dB) advanced per step, sequential only\n"
        "                            (default 1)\n"
        "\n"
        "bandwidth-specific options:\n"
        "  --working-set-size=BYTES  per-thread buffer size, accepts K/M/G suffixes\n"
        "                            (default 1048576; use a size larger than your\n"
        "                            last-level cache to measure real DRAM bandwidth)\n"
        "\n"
        "false-sharing-specific options:\n"
        "  --padding=packed|padded   counter layout (default packed)\n"
        "\n"
        "lock-contention-specific options:\n"
        "  --hold-us=N               busy-work microseconds held per increment\n"
        "                            while holding the shared mutex (default 5.0)\n"
        "\n"
        "sleep-specific options:\n"
        "  --sleep-us=N              nanosleep duration per cycle, in microseconds\n"
        "                            (default 1000.0, i.e. 1ms)\n",
        prog, CYCLELAB_MAX_CHAINS, CYCLELAB_CACHE_LINE_BYTES);
}

void cli_print_version(void) {
    printf("cyclelab 0.1.0 (Thinking in Cycles lab tool)\n");
}

static int parse_double(const char *s, double *out) {
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s || *end != '\0') return -1;
    *out = v;
    return 0;
}

static int parse_long(const char *s, long *out) {
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') return -1;
    *out = v;
    return 0;
}

static int parse_ulong(const char *s, unsigned long *out) {
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 10);
    if (end == s || *end != '\0') return -1;
    *out = v;
    return 0;
}

/* Parses a byte count with an optional trailing K/M/G suffix (base 1024,
 * case-insensitive), e.g. "64K", "16M", "1073741824". */
static int parse_size_bytes(const char *s, long *out) {
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s || v < 0) return -1;
    long multiplier = 1;
    if (*end != '\0') {
        if ((end[0] == 'k' || end[0] == 'K') && end[1] == '\0') multiplier = 1024L;
        else if ((end[0] == 'm' || end[0] == 'M') && end[1] == '\0') multiplier = 1024L * 1024L;
        else if ((end[0] == 'g' || end[0] == 'G') && end[1] == '\0') multiplier = 1024L * 1024L * 1024L;
        else return -1;
    }
    *out = (long)(v * (double)multiplier);
    return 0;
}

/* Splits "--key=value" into key/value pointers into the original string
 * (value may be NULL for bare flags like --help). Returns the key. */
static const char *split_flag(char *arg, char **value) {
    char *eq = strchr(arg, '=');
    if (eq == NULL) {
        *value = NULL;
        return arg;
    }
    *eq = '\0';
    *value = eq + 1;
    return arg;
}

int cli_parse(int argc, char **argv, cyclelab_options_t *opts,
              int *exit_now, int *exit_code) {
    *exit_now = 0;
    *exit_code = 0;
    cli_defaults(opts);

    const char *prog = (argc > 0) ? argv[0] : "cyclelab";

    if (argc < 2) {
        fprintf(stderr, "%s: missing <mode>\n\n", prog);
        cli_print_usage(prog);
        *exit_now = 1;
        *exit_code = 64;
        return -1;
    }

    /* Top-level --help / --version are accepted even before a mode. */
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        cli_print_usage(prog);
        *exit_now = 1;
        *exit_code = 0;
        return 0;
    }
    if (strcmp(argv[1], "--version") == 0) {
        cli_print_version();
        *exit_now = 1;
        *exit_code = 0;
        return 0;
    }

    if (argv[1][0] == '-') {
        fprintf(stderr, "%s: expected <mode> as the first argument, got '%s'\n\n",
                prog, argv[1]);
        cli_print_usage(prog);
        *exit_now = 1;
        *exit_code = 64;
        return -1;
    }

    opts->mode = argv[1];
    if (!mode_is_known(opts->mode)) {
        fprintf(stderr, "%s: unknown mode '%s'\n\n", prog, opts->mode);
        cli_print_usage(prog);
        *exit_now = 1;
        *exit_code = 64;
        return -1;
    }
    /* random-memory's whole point is a random access order; give it that
     * default even though the global default (used by "branch" and
     * "sequential-memory") is SORTED/sequential. An explicit --pattern
     * below still overrides this. */
    if (strcmp(opts->mode, "random-memory") == 0) {
        opts->pattern = CYCLELAB_PATTERN_RANDOM;
    }

    for (int i = 2; i < argc; i++) {
        char *value = NULL;
        const char *key = split_flag(argv[i], &value);

        if (strcmp(key, "--help") == 0 || strcmp(key, "-h") == 0) {
            cli_print_usage(prog);
            *exit_now = 1;
            *exit_code = 0;
            return 0;
        } else if (strcmp(key, "--version") == 0) {
            cli_print_version();
            *exit_now = 1;
            *exit_code = 0;
            return 0;
        } else if (strcmp(key, "--quiet") == 0) {
            opts->quiet = 1;
        } else if (strcmp(key, "--duration") == 0 && value) {
            if (parse_double(value, &opts->duration_s) != 0 || opts->duration_s <= 0) {
                fprintf(stderr, "%s: --duration expects a positive number of seconds\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else if (strcmp(key, "--iterations") == 0 && value) {
            if (parse_long(value, &opts->iterations) != 0 || opts->iterations <= 0) {
                fprintf(stderr, "%s: --iterations expects a positive integer\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else if (strcmp(key, "--threads") == 0 && value) {
            long t = 0;
            if (parse_long(value, &t) != 0 || t <= 0) {
                fprintf(stderr, "%s: --threads expects a positive integer\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
            opts->threads = (int)t;
        } else if (strcmp(key, "--affinity") == 0 && value) {
            opts->affinity_spec = value;
        } else if (strcmp(key, "--seed") == 0 && value) {
            if (parse_ulong(value, &opts->seed) != 0) {
                fprintf(stderr, "%s: --seed expects a non-negative integer\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else if (strcmp(key, "--format") == 0 && value) {
            if (strcmp(value, "json") == 0) opts->format = CYCLELAB_FMT_JSON;
            else if (strcmp(value, "text") == 0) opts->format = CYCLELAB_FMT_TEXT;
            else {
                fprintf(stderr, "%s: --format expects 'json' or 'text'\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else if (strcmp(key, "--output") == 0 && value) {
            opts->output_path = value;
        } else if (strcmp(key, "--op") == 0 && value) {
            if (strcmp(value, "int") == 0) opts->op = CYCLELAB_OP_INT;
            else if (strcmp(value, "float") == 0) opts->op = CYCLELAB_OP_FLOAT;
            else if (strcmp(value, "mixed") == 0) opts->op = CYCLELAB_OP_MIXED;
            else {
                fprintf(stderr, "%s: --op expects 'int', 'float', or 'mixed'\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else if (strcmp(key, "--chains") == 0 && value) {
            long c = 0;
            if (parse_long(value, &c) != 0 || c < 1 || c > CYCLELAB_MAX_CHAINS) {
                fprintf(stderr, "%s: --chains expects an integer from 1 to %d\n",
                        prog, CYCLELAB_MAX_CHAINS);
                *exit_now = 1; *exit_code = 64; return -1;
            }
            opts->chains = (int)c;
        } else if (strcmp(key, "--pattern") == 0 && value) {
            if (strcmp(value, "sorted") == 0 || strcmp(value, "sequential") == 0)
                opts->pattern = CYCLELAB_PATTERN_SORTED;
            else if (strcmp(value, "random") == 0) opts->pattern = CYCLELAB_PATTERN_RANDOM;
            else {
                fprintf(stderr, "%s: --pattern expects 'sorted'/'sequential' or 'random'\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else if (strcmp(key, "--branch-table-size") == 0 && value) {
            if (parse_long(value, &opts->branch_table_size) != 0 || opts->branch_table_size <= 0) {
                fprintf(stderr, "%s: --branch-table-size expects a positive integer\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else if (strcmp(key, "--working-set-size") == 0 && value) {
            if (parse_size_bytes(value, &opts->working_set_bytes) != 0 || opts->working_set_bytes < CYCLELAB_CACHE_LINE_BYTES * 2) {
                fprintf(stderr, "%s: --working-set-size expects a byte count (optionally suffixed K/M/G) of at least %d\n",
                        prog, CYCLELAB_CACHE_LINE_BYTES * 2);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else if (strcmp(key, "--stride") == 0 && value) {
            if (parse_long(value, &opts->mem_stride_slots) != 0 || opts->mem_stride_slots < 1) {
                fprintf(stderr, "%s: --stride expects a positive integer (in cache-line slots)\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else if (strcmp(key, "--padding") == 0 && value) {
            if (strcmp(value, "packed") == 0) opts->padding = CYCLELAB_PADDING_PACKED;
            else if (strcmp(value, "padded") == 0) opts->padding = CYCLELAB_PADDING_PADDED;
            else {
                fprintf(stderr, "%s: --padding expects 'packed' or 'padded'\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else if (strcmp(key, "--hold-us") == 0 && value) {
            if (parse_double(value, &opts->lock_hold_us) != 0 || opts->lock_hold_us < 0) {
                fprintf(stderr, "%s: --hold-us expects a non-negative number of microseconds\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else if (strcmp(key, "--sleep-us") == 0 && value) {
            if (parse_double(value, &opts->sleep_us) != 0 || opts->sleep_us < 0) {
                fprintf(stderr, "%s: --sleep-us expects a non-negative number of microseconds\n", prog);
                *exit_now = 1; *exit_code = 64; return -1;
            }
        } else {
            fprintf(stderr, "%s: unrecognized option '%s'\n\n", prog, argv[i]);
            cli_print_usage(prog);
            *exit_now = 1;
            *exit_code = 64;
            return -1;
        }
    }

    return 0;
}
