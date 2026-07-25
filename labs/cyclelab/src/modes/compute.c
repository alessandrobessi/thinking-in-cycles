#include "compute.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../affinity.h"
#include "../jsonout.h"
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
#define OPS_PER_ITERATION 8

typedef struct {
    int index;
    const cyclelab_options_t *opts;
    double deadline;         /* absolute CLOCK_MONOTONIC seconds; used when opts->iterations == 0 */
    long target_iterations;  /* used when opts->iterations > 0 */

    /* results, filled in by the worker */
    long iterations_done;
    double elapsed_s;
    long long int_checksum;
    double float_checksum;
    cyclelab_affinity_result_t affinity_result;
    const char *affinity_reason;
} compute_worker_ctx_t;

static const char *op_name(cyclelab_op_t op) {
    switch (op) {
        case CYCLELAB_OP_INT: return "int";
        case CYCLELAB_OP_FLOAT: return "float";
        case CYCLELAB_OP_MIXED:
        default: return "mixed";
    }
}

static void *compute_worker(void *arg) {
    compute_worker_ctx_t *ctx = (compute_worker_ctx_t *)arg;

    ctx->affinity_result = affinity_apply(ctx->opts->affinity_spec, ctx->index,
                                           ctx->opts->threads, &ctx->affinity_reason);

    /* Seed depends on both the run seed and the thread index so threads
     * do not all retrace the same accumulator sequence. */
    unsigned long seed = ctx->opts->seed + (unsigned long)ctx->index * 2654435761UL;

    /* Genuine loop-carried dependency: each block's result depends on the
     * previous block's result, so the compiler cannot hoist, reorder away,
     * or constant-fold the loop. `volatile` is belt-and-suspenders on top
     * of that real dependency and the fact that the final values are
     * printed (BLUEPRINT.md Section 8: "prints work completed and a
     * checksum to prevent dead-code elimination"). */
    volatile long long int_acc = (long long)(seed | 1UL);
    volatile double float_acc = 1.0 + (double)(seed % 1000) / 7.0;

    long iters = 0;
    double start = timing_now_seconds();

    for (;;) {
        for (int u = 0; u < OPS_PER_ITERATION; u++) {
            switch (ctx->opts->op) {
                case CYCLELAB_OP_INT:
                    int_acc = int_acc * 2654435761LL + (long long)u + 1;
                    int_acc ^= (int_acc >> 13);
                    break;
                case CYCLELAB_OP_FLOAT:
                    float_acc = float_acc * 1.0000001 + 0.0000003 * (double)(u + 1);
                    if (float_acc > 1e6 || float_acc < -1e6) {
                        float_acc = float_acc - (double)(long long)(float_acc / 1e6) * 1e6;
                    }
                    break;
                case CYCLELAB_OP_MIXED:
                default:
                    int_acc = int_acc * 2654435761LL + (long long)u + 1;
                    int_acc ^= (int_acc >> 13);
                    float_acc = float_acc * 1.0000001 + (double)(int_acc & 0xff) * 1e-7;
                    if (float_acc > 1e6 || float_acc < -1e6) {
                        float_acc = float_acc - (double)(long long)(float_acc / 1e6) * 1e6;
                    }
                    break;
            }
        }
        iters++;

        if (ctx->target_iterations > 0) {
            if (iters >= ctx->target_iterations) break;
        } else {
            /* Check the clock only every 1024 iterations so timing
             * overhead stays negligible relative to the work itself. */
            if ((iters & 0x3FF) == 0 && timing_now_seconds() >= ctx->deadline) break;
        }
    }

    ctx->elapsed_s = timing_now_seconds() - start;
    ctx->iterations_done = iters;
    ctx->int_checksum = int_acc;
    ctx->float_checksum = float_acc;
    return NULL;
}

static uint64_t double_bits(double d) {
    uint64_t u;
    memcpy(&u, &d, sizeof(u));
    return u;
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
                        compute_worker_ctx_t *ctxs, int nthreads,
                        double duration_actual_s, long long total_iterations,
                        uint64_t combined_checksum,
                        const char **warnings, int nwarnings) {
    char started_at[32];
    time_t now = time(NULL);
    struct tm tmv;
    gmtime_r(&now, &tmv);
    strftime(started_at, sizeof(started_at), "%Y-%m-%dT%H:%M:%SZ", &tmv);

    double total_ops = (double)total_iterations * OPS_PER_ITERATION;
    double throughput = (duration_actual_s > 0) ? (total_ops / duration_actual_s) : 0.0;

    fprintf(out, "{\n");
    fprintf(out, "  \"tool\": \"cyclelab\",\n");
    fprintf(out, "  \"version\": \"%s\",\n", CYCLELAB_VERSION);
    fprintf(out, "  \"mode\": \"compute\",\n");
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
        fprintf(out, "    \"iterations_requested\": %ld,\n", opts->iterations);
    } else {
        fprintf(out, "    \"duration_requested_s\": %.6f,\n", opts->duration_s);
        fprintf(out, "    \"iterations_requested\": null,\n");
    }
    fprintf(out, "    \"threads\": %d,\n", opts->threads);
    fprintf(out, "    \"affinity\": \""); json_write_escaped(out, opts->affinity_spec); fprintf(out, "\",\n");
    fprintf(out, "    \"seed\": %lu,\n", opts->seed);
    fprintf(out, "    \"op\": \"%s\"\n", op_name(opts->op));
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
    fprintf(out, "    \"total_iterations\": %lld,\n", total_iterations);
    fprintf(out, "    \"throughput_ops_per_s\": %.2f,\n", throughput);
    fprintf(out, "    \"threads\": [\n");
    for (int i = 0; i < nthreads; i++) {
        fprintf(out, "      {\n");
        fprintf(out, "        \"index\": %d,\n", ctxs[i].index);
        fprintf(out, "        \"iterations\": %ld,\n", ctxs[i].iterations_done);
        fprintf(out, "        \"elapsed_s\": %.6f,\n", ctxs[i].elapsed_s);
        fprintf(out, "        \"affinity_applied\": %s,\n",
                (ctxs[i].affinity_result == CYCLELAB_AFFINITY_APPLIED) ? "true" : "false");
        fprintf(out, "        \"checksum\": \"%016llx%016llx\"\n",
                (unsigned long long)ctxs[i].int_checksum,
                (unsigned long long)double_bits(ctxs[i].float_checksum));
        fprintf(out, "      }%s\n", (i == nthreads - 1) ? "" : ",");
    }
    fprintf(out, "    ],\n");
    fprintf(out, "    \"combined_checksum\": \"%016llx\"\n", (unsigned long long)combined_checksum);
    fprintf(out, "  }\n");
    fprintf(out, "}\n");
}

static void print_text(FILE *out, const cyclelab_options_t *opts,
                        const cyclelab_hostinfo_t *host,
                        compute_worker_ctx_t *ctxs, int nthreads,
                        double duration_actual_s, long long total_iterations,
                        uint64_t combined_checksum,
                        const char **warnings, int nwarnings) {
    double total_ops = (double)total_iterations * OPS_PER_ITERATION;
    double throughput = (duration_actual_s > 0) ? (total_ops / duration_actual_s) : 0.0;

    fprintf(out, "cyclelab compute -- %s/%s, %d logical CPUs, build=%s\n",
            host->os, host->arch, host->logical_cpus, CYCLELAB_BUILD_TYPE);
    fprintf(out, "threads=%d op=%s seed=%lu\n", opts->threads, op_name(opts->op), opts->seed);
    for (int i = 0; i < nwarnings; i++) {
        fprintf(out, "warning: %s\n", warnings[i]);
    }
    fprintf(out, "duration_actual_s=%.6f total_iterations=%lld throughput_ops_per_s=%.2f\n",
            duration_actual_s, total_iterations, throughput);
    for (int i = 0; i < nthreads; i++) {
        fprintf(out, "  thread[%d] iterations=%ld elapsed_s=%.6f affinity_applied=%s\n",
                ctxs[i].index, ctxs[i].iterations_done, ctxs[i].elapsed_s,
                (ctxs[i].affinity_result == CYCLELAB_AFFINITY_APPLIED) ? "true" : "false");
    }
    fprintf(out, "combined_checksum=%016llx\n", (unsigned long long)combined_checksum);
}

int compute_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host) {
    int nthreads = opts->threads;
    compute_worker_ctx_t *ctxs = calloc((size_t)nthreads, sizeof(compute_worker_ctx_t));
    pthread_t *tids = calloc((size_t)nthreads, sizeof(pthread_t));
    if (ctxs == NULL || tids == NULL) {
        fprintf(stderr, "cyclelab: out of memory allocating %d thread contexts\n", nthreads);
        free(ctxs);
        free(tids);
        return 1;
    }

    double start_wall = timing_now_seconds();
    double deadline = start_wall + opts->duration_s;

    for (int i = 0; i < nthreads; i++) {
        ctxs[i].index = i;
        ctxs[i].opts = opts;
        ctxs[i].deadline = deadline;
        ctxs[i].target_iterations = opts->iterations;
    }

    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&tids[i], NULL, compute_worker, &ctxs[i]) != 0) {
            fprintf(stderr, "cyclelab: failed to start worker thread %d\n", i);
            for (int j = 0; j < i; j++) pthread_join(tids[j], NULL);
            free(ctxs);
            free(tids);
            return 1;
        }
    }
    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }
    double duration_actual_s = timing_now_seconds() - start_wall;

    long long total_iterations = 0;
    uint64_t combined_checksum = 0;
    for (int i = 0; i < nthreads; i++) {
        total_iterations += ctxs[i].iterations_done;
        uint64_t int_bits = (uint64_t)ctxs[i].int_checksum;
        uint64_t float_bits = double_bits(ctxs[i].float_checksum);
        combined_checksum ^= mix64(int_bits ^ mix64(float_bits) ^ (uint64_t)i);
    }

    /* Deduplicate affinity warning strings by pointer -- affinity_apply()
     * always returns the same string literal for the same failure reason,
     * so pointer equality is sufficient and avoids a strcmp pass. */
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
            free(ctxs);
            free(tids);
            return 1;
        }
        close_out = 1;
    }

    if (opts->format == CYCLELAB_FMT_JSON) {
        print_json(out, opts, host, ctxs, nthreads, duration_actual_s, total_iterations,
                   combined_checksum, warnings, nwarnings);
    } else {
        print_text(out, opts, host, ctxs, nthreads, duration_actual_s, total_iterations,
                   combined_checksum, warnings, nwarnings);
    }

    if (close_out) fclose(out);
    free(ctxs);
    free(tids);
    return 0;
}
