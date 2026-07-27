#include "bandwidth.h"

#include <pthread.h>
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

/* A minimal reusable barrier: macOS's libpthread does not provide
 * pthread_barrier_t at all (Linux's does), so this mode needs its own to
 * hold every worker at the starting line until all of them have finished
 * allocating and first-touching their own buffer (see bw_worker below). */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int total;
    int generation;
} bw_barrier_t;

static void bw_barrier_init(bw_barrier_t *b, int total) {
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->count = 0;
    b->total = total;
    b->generation = 0;
}

static void bw_barrier_wait(bw_barrier_t *b) {
    pthread_mutex_lock(&b->mutex);
    int gen = b->generation;
    b->count++;
    if (b->count == b->total) {
        b->generation++;
        b->count = 0;
        pthread_cond_broadcast(&b->cond);
    } else {
        while (gen == b->generation) {
            pthread_cond_wait(&b->cond, &b->mutex);
        }
    }
    pthread_mutex_unlock(&b->mutex);
}

static void bw_barrier_destroy(bw_barrier_t *b) {
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->cond);
}

typedef struct {
    int index;
    const cyclelab_options_t *opts;
    double *buf;
    long num_doubles;
    bw_barrier_t *barrier;
    int *any_alloc_failed;
    long target_passes;

    long passes_done;
    double elapsed_s;
    double checksum;
    cyclelab_affinity_result_t affinity_result;
    const char *affinity_reason;
} bw_worker_ctx_t;

static void *bw_worker(void *arg) {
    bw_worker_ctx_t *ctx = (bw_worker_ctx_t *)arg;
    ctx->affinity_result = affinity_apply(ctx->opts->affinity_spec, ctx->index,
                                           ctx->opts->threads, &ctx->affinity_reason);

    /* Allocate and first-touch *after* affinity is applied, and inside
     * this worker rather than the main thread: on a NUMA system, the
     * thread that first writes a page is the thread whose node that page
     * lands on (Chapter 25's own subject). Having the main thread
     * allocate and initialize every worker's buffer up front, before any
     * worker's affinity was even applied, would place all of it on
     * whichever node the main thread happened to be running on --
     * exactly the confound this mode exists to avoid, since its whole
     * point is measuring bandwidth from each worker's own local memory. */
    long n = ctx->num_doubles;
    double *buf = malloc((size_t)n * sizeof(double));
    if (buf == NULL) {
        *ctx->any_alloc_failed = 1;
        bw_barrier_wait(ctx->barrier);
        return NULL;
    }
    for (long k = 0; k < n; k++) {
        buf[k] = (double)((k * 2654435761UL + (unsigned long)ctx->index) % 1000) / 7.0;
    }
    ctx->buf = buf;

    /* Every worker crosses this barrier only once its own allocation and
     * first-touch is done, so the timed loop below starts from the same
     * line for every thread instead of some threads racing ahead while
     * others are still faulting in pages. */
    bw_barrier_wait(ctx->barrier);
    if (*ctx->any_alloc_failed) return NULL;

    double deadline = timing_now_seconds() + ctx->opts->duration_s;
    volatile double sum = 0.0;
    long passes = 0;
    double start = timing_now_seconds();

    for (;;) {
        /* A plain, independent-iteration reduction: nothing here forces
         * one iteration to wait on another, so the compiler is free to
         * vectorize it and the CPU is free to prefetch aggressively --
         * exactly what a bandwidth measurement needs, unlike the
         * deliberately dependent pointer chase in sequential-memory/
         * random-memory mode. */
        double local = 0.0;
        for (long i = 0; i < n; i++) {
            local += buf[i];
        }
        sum += local;
        passes++;

        if (ctx->target_passes > 0) {
            if (passes >= ctx->target_passes) break;
        } else {
            if (timing_now_seconds() >= deadline) break;
        }
    }

    ctx->elapsed_s = timing_now_seconds() - start;
    ctx->passes_done = passes;
    ctx->checksum = sum;
    return NULL;
}

static void print_json(FILE *out, const cyclelab_options_t *opts,
                        const cyclelab_hostinfo_t *host,
                        bw_worker_ctx_t *ctxs, int nthreads,
                        double duration_actual_s, long long total_passes,
                        long long total_bytes, long ctx_nvcsw, long ctx_nivcsw,
                        const char **warnings, int nwarnings) {
    char started_at[32];
    time_t now = time(NULL);
    struct tm tmv;
    gmtime_r(&now, &tmv);
    strftime(started_at, sizeof(started_at), "%Y-%m-%dT%H:%M:%SZ", &tmv);

    double bandwidth_bytes_per_s = (duration_actual_s > 0) ? ((double)total_bytes / duration_actual_s) : 0.0;
    double bandwidth_gb_per_s = bandwidth_bytes_per_s / 1e9;

    fprintf(out, "{\n");
    fprintf(out, "  \"tool\": \"cyclelab\",\n");
    fprintf(out, "  \"version\": \"%s\",\n", CYCLELAB_VERSION);
    fprintf(out, "  \"mode\": \"bandwidth\",\n");
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
        fprintf(out, "    \"passes_requested\": %ld,\n", opts->iterations);
    } else {
        fprintf(out, "    \"duration_requested_s\": %.6f,\n", opts->duration_s);
        fprintf(out, "    \"passes_requested\": null,\n");
    }
    fprintf(out, "    \"threads\": %d,\n", opts->threads);
    fprintf(out, "    \"affinity\": \""); json_write_escaped(out, opts->affinity_spec); fprintf(out, "\",\n");
    fprintf(out, "    \"working_set_bytes\": %ld\n", opts->working_set_bytes);
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
    fprintf(out, "    \"total_passes\": %lld,\n", total_passes);
    fprintf(out, "    \"total_bytes_read\": %lld,\n", total_bytes);
    fprintf(out, "    \"bandwidth_gb_per_s\": %.3f,\n", bandwidth_gb_per_s);
    fprintf(out, "    \"threads\": [\n");
    for (int i = 0; i < nthreads; i++) {
        fprintf(out, "      {\n");
        fprintf(out, "        \"index\": %d,\n", ctxs[i].index);
        fprintf(out, "        \"passes\": %ld,\n", ctxs[i].passes_done);
        fprintf(out, "        \"elapsed_s\": %.6f,\n", ctxs[i].elapsed_s);
        fprintf(out, "        \"affinity_applied\": %s\n",
                (ctxs[i].affinity_result == CYCLELAB_AFFINITY_APPLIED) ? "true" : "false");
        fprintf(out, "      }%s\n", (i == nthreads - 1) ? "" : ",");
    }
    fprintf(out, "    ],\n");
    fprintf(out, "    \"context_switches\": { \"voluntary\": %ld, \"involuntary\": %ld }\n",
            ctx_nvcsw, ctx_nivcsw);
    fprintf(out, "  }\n");
    fprintf(out, "}\n");
}

static void print_text(FILE *out, const cyclelab_options_t *opts,
                        const cyclelab_hostinfo_t *host,
                        bw_worker_ctx_t *ctxs, int nthreads,
                        double duration_actual_s, long long total_passes,
                        long long total_bytes, long ctx_nvcsw, long ctx_nivcsw,
                        const char **warnings, int nwarnings) {
    double bandwidth_gb_per_s = (duration_actual_s > 0) ? ((double)total_bytes / duration_actual_s / 1e9) : 0.0;

    fprintf(out, "cyclelab bandwidth -- %s/%s, %d logical CPUs, build=%s\n",
            host->os, host->arch, host->logical_cpus, CYCLELAB_BUILD_TYPE);
    fprintf(out, "threads=%d working_set_bytes=%ld\n", opts->threads, opts->working_set_bytes);
    for (int i = 0; i < nwarnings; i++) {
        fprintf(out, "warning: %s\n", warnings[i]);
    }
    fprintf(out, "duration_actual_s=%.6f total_passes=%lld total_bytes_read=%lld bandwidth_gb_per_s=%.3f\n",
            duration_actual_s, total_passes, total_bytes, bandwidth_gb_per_s);
    for (int i = 0; i < nthreads; i++) {
        fprintf(out, "  thread[%d] passes=%ld elapsed_s=%.6f affinity_applied=%s\n",
                ctxs[i].index, ctxs[i].passes_done, ctxs[i].elapsed_s,
                (ctxs[i].affinity_result == CYCLELAB_AFFINITY_APPLIED) ? "true" : "false");
    }
    fprintf(out, "context_switches voluntary=%ld involuntary=%ld\n", ctx_nvcsw, ctx_nivcsw);
}

int bandwidth_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host) {
    int nthreads = opts->threads;
    bw_worker_ctx_t *ctxs = calloc((size_t)nthreads, sizeof(bw_worker_ctx_t));
    pthread_t *tids = calloc((size_t)nthreads, sizeof(pthread_t));
    if (ctxs == NULL || tids == NULL) {
        fprintf(stderr, "cyclelab: out of memory allocating %d thread contexts\n", nthreads);
        free(ctxs);
        free(tids);
        return 1;
    }

    long num_doubles = opts->working_set_bytes / (long)sizeof(double);
    if (num_doubles < 1) num_doubles = 1;

    /* nthreads + 1: the main thread is itself a barrier participant, so
     * it can capture start_wall at the exact moment every worker has
     * finished allocating and first-touching its own buffer -- excluding
     * that allocation time from the measured duration below, rather than
     * folding it into the reported bandwidth number. */
    bw_barrier_t barrier;
    bw_barrier_init(&barrier, nthreads + 1);
    int any_alloc_failed = 0;

    for (int i = 0; i < nthreads; i++) {
        ctxs[i].index = i;
        ctxs[i].opts = opts;
        ctxs[i].num_doubles = num_doubles;
        ctxs[i].barrier = &barrier;
        ctxs[i].any_alloc_failed = &any_alloc_failed;
        ctxs[i].target_passes = opts->iterations;
    }

    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&tids[i], NULL, bw_worker, &ctxs[i]) != 0) {
            fprintf(stderr, "cyclelab: failed to start worker thread %d\n", i);
            for (int j = 0; j < i; j++) pthread_join(tids[j], NULL);
            for (int j = 0; j < nthreads; j++) free(ctxs[j].buf);
            bw_barrier_destroy(&barrier);
            free(ctxs);
            free(tids);
            return 1;
        }
    }

    bw_barrier_wait(&barrier); /* released once every worker has allocated */
    double start_wall = timing_now_seconds();

    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }
    double duration_actual_s = timing_now_seconds() - start_wall;
    bw_barrier_destroy(&barrier);

    if (any_alloc_failed) {
        fprintf(stderr, "cyclelab: out of memory allocating a %ld-byte buffer for one or more threads\n",
                opts->working_set_bytes);
        for (int i = 0; i < nthreads; i++) free(ctxs[i].buf);
        free(ctxs);
        free(tids);
        return 1;
    }

    long long total_passes = 0;
    long long total_bytes = 0;
    for (int i = 0; i < nthreads; i++) {
        total_passes += ctxs[i].passes_done;
        total_bytes += (long long)ctxs[i].passes_done * ctxs[i].num_doubles * (long long)sizeof(double);
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
            for (int i = 0; i < nthreads; i++) free(ctxs[i].buf);
            free(ctxs);
            free(tids);
            return 1;
        }
        close_out = 1;
    }

    long ctx_nvcsw, ctx_nivcsw;
    rusage_get_context_switches(&ctx_nvcsw, &ctx_nivcsw);

    if (opts->format == CYCLELAB_FMT_JSON) {
        print_json(out, opts, host, ctxs, nthreads, duration_actual_s, total_passes, total_bytes,
                   ctx_nvcsw, ctx_nivcsw, warnings, nwarnings);
    } else {
        print_text(out, opts, host, ctxs, nthreads, duration_actual_s, total_passes, total_bytes,
                   ctx_nvcsw, ctx_nivcsw, warnings, nwarnings);
    }

    if (close_out) fclose(out);
    for (int i = 0; i < nthreads; i++) free(ctxs[i].buf);
    free(ctxs);
    free(tids);
    return 0;
}
