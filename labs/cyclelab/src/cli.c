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
}

void cli_print_usage(const char *prog) {
    fprintf(stderr,
        "usage: %s <mode> [options]\n"
        "\n"
        "modes:\n"
        "  compute               implemented\n"
        "  branch, sequential-memory, random-memory, bandwidth,\n"
        "  false-sharing, lock-contention, syscall, sleep, numa, mixed\n"
        "                        recognized, not yet implemented\n"
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
        "  --op=int|float|mixed  which accumulator chain to run (default mixed)\n",
        prog);
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
