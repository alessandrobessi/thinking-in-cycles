#include "branch.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../affinity.h"
#include "../jsonout.h"
#include "../rusage_util.h"
#include "../timing.h"

#ifndef CYCLELAB_BUILD_TYPE
#define CYCLELAB_BUILD_TYPE "unknown"
#endif
#ifndef CYCLELAB_CFLAGS
#define CYCLELAB_CFLAGS "unknown"
#endif

#if defined(__clang__)
#define CYCLELAB_COMPILER "clang " __VERSION__
#elif defined(__GNUC__)
#define CYCLELAB_COMPILER "gcc " __VERSION__
#else
#define CYCLELAB_COMPILER "unknown"
#endif

#define CYCLELAB_VERSION "0.1.0"

typedef struct {
    int index;
    const cyclelab_options_t *opts;
    double deadline;        /* used when opts->iterations == 0 */
    long target_passes;     /* used when opts->iterations > 0: full table passes */
    unsigned char *table;
    long table_size;

    long passes_done;
    double elapsed_s;
    long long checksum;
    cyclelab_affinity_result_t affinity_result;
    const char *affinity_reason;
} branch_worker_ctx_t;

static const char *pattern_name(cyclelab_pattern_t p) {
    return (p == CYCLELAB_PATTERN_SORTED) ? "sorted" : "random";
}

/* Small, self-contained xorshift64 PRNG so table construction is
 * deterministic from --seed without depending on libc's rand()/rand_r()
 * (whose quality and thread-safety guarantees vary by platform). */
static uint64_t xorshift64(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static int compare_uchar(const void *a, const void *b) {
    return (int)(*(const unsigned char *)a) - (int)(*(const unsigned char *)b);
}

/* Builds one thread's table of pseudo-random byte values 0-255. In
 * "sorted" order, long runs of the same branch outcome sit next to each
 * other -- easy for a branch predictor. In "random" order, the outcome of
 * `value >= 128` flips unpredictably from one element to the next. */
static unsigned char *build_table(long n, unsigned long seed, cyclelab_pattern_t pattern) {
    unsigned char *table = malloc((size_t)n);
    if (table == NULL) return NULL;

    uint64_t state = seed ? (uint64_t)seed : 1;
    for (long i = 0; i < n; i++) {
        table[i] = (unsigned char)(xorshift64(&state) & 0xFFu);
    }
    if (pattern == CYCLELAB_PATTERN_SORTED) {
        qsort(table, (size_t)n, sizeof(unsigned char), compare_uchar);
    }
    return table;
}

static void *branch_worker(void *arg) {
    branch_worker_ctx_t *ctx = (branch_worker_ctx_t *)arg;

    ctx->affinity_result = affinity_apply(ctx->opts->affinity_spec, ctx->index,
                                           ctx->opts->threads, &ctx->affinity_reason);

    /* The data-dependent conditional this mode exists to exercise: which
     * accumulator gets each element depends on the element's value, so the
     * branch outcome is genuinely tied to the table's contents and cannot
     * be predicted by the compiler at build time. `volatile` plus printing
     * the final folded checksum keeps this from being optimized away
     * (BLUEPRINT.md Section 8). */
    volatile long long sum_high = 0; /* values >= 128 */
    volatile long long sum_low = 0;  /* values < 128 */

    long passes = 0;
    double start = timing_now_seconds();

    for (;;) {
        for (long i = 0; i < ctx->table_size; i++) {
            unsigned char v = ctx->table[i];
            if (v >= 128) {
                sum_high += v;
            } else {
                sum_low += v;
            }
        }
        passes++;

        if (ctx->target_passes > 0) {
            if (passes >= ctx->target_passes) break;
        } else {
            /* One full table pass is already a substantial unit of work
             * (--branch-table-size defaults to 1,000,000), so checking the
             * clock once per pass keeps timing overhead negligible. */
            if (timing_now_seconds() >= ctx->deadline) break;
        }
    }

    ctx->elapsed_s = timing_now_seconds() - start;
    ctx->passes_done = passes;
    ctx->checksum = (sum_high * 2654435761LL) ^ sum_low;
    return NULL;
}

static uint64_t mix64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

static void print_json(FILE *out, const cyclelab_options_t *opts,
                        const cyclelab_hostinfo_t *host,
                        branch_worker_ctx_t *ctxs, int nthreads,
                        double duration_actual_s, long long total_passes,
                        long long total_elements, uint64_t combined_checksum,
                        long ctx_nvcsw, long ctx_nivcsw,
                        const char **warnings, int nwarnings) {
    char started_at[32];
    time_t now = time(NULL);
    struct tm tmv;
    gmtime_r(&now, &tmv);
    strftime(started_at, sizeof(started_at), "%Y-%m-%dT%H:%M:%SZ", &tmv);

    double throughput = (duration_actual_s > 0) ? ((double)total_elements / duration_actual_s) : 0.0;

    fprintf(out, "{\n");
    fprintf(out, "  \"tool\": \"cyclelab\",\n");
    fprintf(out, "  \"version\": \"%s\",\n", CYCLELAB_VERSION);
    fprintf(out, "  \"mode\": \"branch\",\n");
    fprintf(out, "  \"started_at\": \"%s\",\n", started_at);

    fprintf(out, "  \"build\": {\n");
    fprintf(out, "    \"type\": \"%s\",\n", CYCLELAB_BUILD_TYPE);
    fprintf(out, "    \"cflags\": \"%s\",\n", CYCLELAB_CFLAGS);
    fprintf(out, "    \"compiler\": \"%s\"\n", CYCLELAB_COMPILER);
    fprintf(out, "  },\n");

    fprintf(out, "  \"host\": {\n");
    fprintf(out, "    \"os\": \""); json_write_escaped(out, host->os); fprintf(out, "\",\n");
    fprintf(out, "    \"kernel\": \""); json_write_escaped(out, host->kernel); fprintf(out, "\",\n");
    fprintf(out, "    \"arch\": \""); json_write_escaped(out, host->arch); fprintf(out, "\",\n");
    fprintf(out, "    \"hostname\": \""); json_write_escaped(out, host->hostname); fprintf(out, "\",\n");
    fprintf(out, "    \"logical_cpus\": %d\n", host->logical_cpus);
    fprintf(out, "  },\n");

    fprintf(out, "  \"config\": {\n");
    if (opts->iterations > 0) {
        fprintf(out, "    \"duration_requested_s\": null,\n");
        fprintf(out, "    \"table_passes_requested\": %ld,\n", opts->iterations);
    } else {
        fprintf(out, "    \"duration_requested_s\": %.6f,\n", opts->duration_s);
        fprintf(out, "    \"table_passes_requested\": null,\n");
    }
    fprintf(out, "    \"threads\": %d,\n", opts->threads);
    fprintf(out, "    \"affinity\": \""); json_write_escaped(out, opts->affinity_spec); fprintf(out, "\",\n");
    fprintf(out, "    \"seed\": %lu,\n", opts->seed);
    fprintf(out, "    \"pattern\": \"%s\",\n", pattern_name(opts->pattern));
    fprintf(out, "    \"branch_table_size\": %ld\n", opts->branch_table_size);
    fprintf(out, "  },\n");

    fprintf(out, "  \"warnings\": [");
    for (int i = 0; i < nwarnings; i++) {
        fprintf(out, "%s\n    \"", (i == 0) ? "" : ",");
        json_write_escaped(out, warnings[i]);
        fprintf(out, "\"");
    }
    fprintf(out, "%s],\n", (nwarnings > 0) ? "\n  " : "");

    fprintf(out, "  \"results\": {\n");
    fprintf(out, "    \"duration_actual_s\": %.6f,\n", duration_actual_s);
    fprintf(out, "    \"total_table_passes\": %lld,\n", total_passes);
    fprintf(out, "    \"total_elements_processed\": %lld,\n", total_elements);
    fprintf(out, "    \"throughput_elements_per_s\": %.2f,\n", throughput);
    fprintf(out, "    \"threads\": [\n");
    for (int i = 0; i < nthreads; i++) {
        fprintf(out, "      {\n");
        fprintf(out, "        \"index\": %d,\n", ctxs[i].index);
        fprintf(out, "        \"table_passes\": %ld,\n", ctxs[i].passes_done);
        fprintf(out, "        \"elapsed_s\": %.6f,\n", ctxs[i].elapsed_s);
        fprintf(out, "        \"affinity_applied\": %s,\n",
                (ctxs[i].affinity_result == CYCLELAB_AFFINITY_APPLIED) ? "true" : "false");
        fprintf(out, "        \"checksum\": \"%016llx\"\n", (unsigned long long)ctxs[i].checksum);
        fprintf(out, "      }%s\n", (i == nthreads - 1) ? "" : ",");
    }
    fprintf(out, "    ],\n");
    fprintf(out, "    \"combined_checksum\": \"%016llx\",\n", (unsigned long long)combined_checksum);
    fprintf(out, "    \"context_switches\": { \"voluntary\": %ld, \"involuntary\": %ld }\n",
            ctx_nvcsw, ctx_nivcsw);
    fprintf(out, "  }\n");
    fprintf(out, "}\n");
}

static void print_text(FILE *out, const cyclelab_options_t *opts,
                        const cyclelab_hostinfo_t *host,
                        branch_worker_ctx_t *ctxs, int nthreads,
                        double duration_actual_s, long long total_passes,
                        long long total_elements, uint64_t combined_checksum,
                        long ctx_nvcsw, long ctx_nivcsw,
                        const char **warnings, int nwarnings) {
    double throughput = (duration_actual_s > 0) ? ((double)total_elements / duration_actual_s) : 0.0;

    fprintf(out, "cyclelab branch -- %s/%s, %d logical CPUs, build=%s\n",
            host->os, host->arch, host->logical_cpus, CYCLELAB_BUILD_TYPE);
    fprintf(out, "threads=%d pattern=%s table_size=%ld seed=%lu\n",
            opts->threads, pattern_name(opts->pattern), opts->branch_table_size, opts->seed);
    for (int i = 0; i < nwarnings; i++) {
        fprintf(out, "warning: %s\n", warnings[i]);
    }
    fprintf(out, "duration_actual_s=%.6f total_table_passes=%lld total_elements_processed=%lld throughput_elements_per_s=%.2f\n",
            duration_actual_s, total_passes, total_elements, throughput);
    for (int i = 0; i < nthreads; i++) {
        fprintf(out, "  thread[%d] table_passes=%ld elapsed_s=%.6f affinity_applied=%s\n",
                ctxs[i].index, ctxs[i].passes_done, ctxs[i].elapsed_s,
                (ctxs[i].affinity_result == CYCLELAB_AFFINITY_APPLIED) ? "true" : "false");
    }
    fprintf(out, "combined_checksum=%016llx\n", (unsigned long long)combined_checksum);
    fprintf(out, "context_switches voluntary=%ld involuntary=%ld\n", ctx_nvcsw, ctx_nivcsw);
}

int branch_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host) {
    int nthreads = opts->threads;
    branch_worker_ctx_t *ctxs = calloc((size_t)nthreads, sizeof(branch_worker_ctx_t));
    pthread_t *tids = calloc((size_t)nthreads, sizeof(pthread_t));
    if (ctxs == NULL || tids == NULL) {
        fprintf(stderr, "cyclelab: out of memory allocating %d thread contexts\n", nthreads);
        free(ctxs);
        free(tids);
        return 1;
    }

    for (int i = 0; i < nthreads; i++) {
        unsigned long table_seed = opts->seed + (unsigned long)i * 2654435761UL;
        ctxs[i].table = build_table(opts->branch_table_size, table_seed, opts->pattern);
        if (ctxs[i].table == NULL) {
            fprintf(stderr, "cyclelab: out of memory building a %ld-element table for thread %d\n",
                    opts->branch_table_size, i);
            for (int j = 0; j < i; j++) free(ctxs[j].table);
            free(ctxs);
            free(tids);
            return 1;
        }
        ctxs[i].table_size = opts->branch_table_size;
    }

    double start_wall = timing_now_seconds();
    double deadline = start_wall + opts->duration_s;

    for (int i = 0; i < nthreads; i++) {
        ctxs[i].index = i;
        ctxs[i].opts = opts;
        ctxs[i].deadline = deadline;
        ctxs[i].target_passes = opts->iterations;
    }

    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&tids[i], NULL, branch_worker, &ctxs[i]) != 0) {
            fprintf(stderr, "cyclelab: failed to start worker thread %d\n", i);
            for (int j = 0; j < i; j++) pthread_join(tids[j], NULL);
            for (int j = 0; j < nthreads; j++) free(ctxs[j].table);
            free(ctxs);
            free(tids);
            return 1;
        }
    }
    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }
    double duration_actual_s = timing_now_seconds() - start_wall;

    long long total_passes = 0;
    long long total_elements = 0;
    uint64_t combined_checksum = 0;
    for (int i = 0; i < nthreads; i++) {
        total_passes += ctxs[i].passes_done;
        total_elements += (long long)ctxs[i].passes_done * ctxs[i].table_size;
        combined_checksum ^= mix64((uint64_t)ctxs[i].checksum ^ (uint64_t)i);
    }

    const char *warnings[64];
    int nwarnings = 0;
    for (int i = 0; i < nthreads && nwarnings < 64; i++) {
        if (ctxs[i].affinity_result == CYCLELAB_AFFINITY_UNSUPPORTED && ctxs[i].affinity_reason) {
            int already = 0;
            for (int w = 0; w < nwarnings; w++) {
                if (warnings[w] == ctxs[i].affinity_reason) { already = 1; break; }
            }
            if (!already) warnings[nwarnings++] = ctxs[i].affinity_reason;
        }
    }
    if (!opts->quiet) {
        for (int w = 0; w < nwarnings; w++) {
            fprintf(stderr, "cyclelab: warning: %s\n", warnings[w]);
        }
    }

    FILE *out = stdout;
    int close_out = 0;
    if (strcmp(opts->output_path, "-") != 0) {
        out = fopen(opts->output_path, "w");
        if (out == NULL) {
            fprintf(stderr, "cyclelab: could not open output file '%s'\n", opts->output_path);
            for (int i = 0; i < nthreads; i++) free(ctxs[i].table);
            free(ctxs);
            free(tids);
            return 1;
        }
        close_out = 1;
    }

    long ctx_nvcsw, ctx_nivcsw;
    rusage_get_context_switches(&ctx_nvcsw, &ctx_nivcsw);

    if (opts->format == CYCLELAB_FMT_JSON) {
        print_json(out, opts, host, ctxs, nthreads, duration_actual_s, total_passes,
                   total_elements, combined_checksum, ctx_nvcsw, ctx_nivcsw, warnings, nwarnings);
    } else {
        print_text(out, opts, host, ctxs, nthreads, duration_actual_s, total_passes,
                   total_elements, combined_checksum, ctx_nvcsw, ctx_nivcsw, warnings, nwarnings);
    }

    if (close_out) fclose(out);
    for (int i = 0; i < nthreads; i++) free(ctxs[i].table);
    free(ctxs);
    free(tids);
    return 0;
}
