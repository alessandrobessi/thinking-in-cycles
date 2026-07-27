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

/* A ready gate, not a reusable barrier: every worker allocates and
 * first-touches its own buffer (see bw_worker below), marks itself
 * ready, and then blocks until the main thread has published one
 * shared start_time/deadline and released everyone at once -- a real
 * common measurement window, rather than each worker computing its own
 * deadline from its own wake-up time (which can drift under scheduling
 * jitter, especially oversubscribed). A failure anywhere -- a worker's
 * allocation, or main's own pthread_create() -- sets abort, which wakes
 * every waiting participant immediately instead of leaving already-
 * created workers blocked on a ready count that can now never be
 * reached. All of ready_count/abort/go/start_time/deadline are only
 * ever touched with mutex held, except for the post-release reads in
 * bw_worker, which happen only after this thread's own matching
 * lock/unlock pair below -- safe by the usual happens-before argument. */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int ready_count;
    int total;
    int abort;
    int go;
    double start_time;
    double deadline; /* meaningful only when opts->iterations == 0 */
} bw_gate_t;

static void bw_gate_init(bw_gate_t *g, int total) {
    pthread_mutex_init(&g->mutex, NULL);
    pthread_cond_init(&g->cond, NULL);
    g->ready_count = 0;
    g->total = total;
    g->abort = 0;
    g->go = 0;
    g->start_time = 0.0;
    g->deadline = 0.0;
}

static void bw_gate_destroy(bw_gate_t *g) {
    pthread_mutex_destroy(&g->mutex);
    pthread_cond_destroy(&g->cond);
}

/* Worker side: mark ready (or aborted), then wait for release. */
static void bw_gate_worker_ready_and_wait(bw_gate_t *g, int failed) {
    pthread_mutex_lock(&g->mutex);
    if (failed) g->abort = 1;
    g->ready_count++;
    pthread_cond_broadcast(&g->cond); /* wake main if it's waiting on ready_count */
    while (!g->go && !g->abort) {
        pthread_cond_wait(&g->cond, &g->mutex);
    }
    pthread_mutex_unlock(&g->mutex);
}

/* Main side, once every worker thread has been successfully created:
 * wait for all of them to finish allocating, then either abort (one of
 * them failed to allocate) or publish a shared start_time/deadline and
 * release everyone from the same starting line at once. Returns nonzero
 * if the run was aborted. */
static int bw_gate_main_release(bw_gate_t *g, double duration_s) {
    pthread_mutex_lock(&g->mutex);
    while (g->ready_count < g->total && !g->abort) {
        pthread_cond_wait(&g->cond, &g->mutex);
    }
    int aborted = g->abort;
    if (!aborted) {
        g->start_time = timing_now_seconds();
        g->deadline = g->start_time + duration_s;
        g->go = 1;
    }
    pthread_cond_broadcast(&g->cond);
    pthread_mutex_unlock(&g->mutex);
    return aborted;
}

/* Main side, only on a pthread_create() failure: aborts the gate so any
 * already-created workers currently waiting in
 * bw_gate_worker_ready_and_wait can return instead of blocking forever
 * on a ready count that will now never reach `total`. */
static void bw_gate_abort(bw_gate_t *g) {
    pthread_mutex_lock(&g->mutex);
    g->abort = 1;
    pthread_cond_broadcast(&g->cond);
    pthread_mutex_unlock(&g->mutex);
}

typedef struct {
    int index;
    const cyclelab_options_t *opts;
    double *buf;
    long num_doubles;
    bw_gate_t *gate;
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
    int failed = (buf == NULL);
    if (!failed) {
        for (long k = 0; k < n; k++) {
            buf[k] = (double)((k * 2654435761UL + (unsigned long)ctx->index) % 1000) / 7.0;
        }
        ctx->buf = buf;
    }

    bw_gate_worker_ready_and_wait(ctx->gate, failed);
    if (ctx->gate->abort) return NULL;

    /* start_time/deadline were published once by main, under its own
     * lock, before this thread's own bw_gate_worker_ready_and_wait call
     * returned -- a shared absolute window every worker times against,
     * not a value each worker computes independently from its own
     * wake-up time. */
    double deadline = ctx->gate->deadline;
    volatile double sum = 0.0;
    long passes = 0;
    double start = ctx->gate->start_time;

    for (;;) {
        /* Eight independent partial sums, not one running total: a
         * single `local += buf[i]` accumulator is one serial
         * floating-point dependency chain (every add waits on the
         * previous add's result), and without -ffast-math/-fassociative-
         * math -- which this project does not build with, since it would
         * silently change every other mode's arithmetic too -- the
         * compiler is not free to reassociate that chain into a
         * vectorized reduction; strict IEEE-754 semantics require the
         * additions to happen in exactly the written order. Eight
         * mutually-independent accumulators break that single chain into
         * eight, each only depending on its own previous value. Confirmed
         * in this exact build's generated assembly (labs/cyclelab/README.md's
         * bandwidth section): the compiler vectorizes this loop into four
         * independent 2-wide NEON accumulators (`fadd.2d`) loading 8
         * doubles per iteration, versus zero vectorization and one
         * scalar `fadd` chain for the single-accumulator version this
         * replaced. The final reduction of the eight partial sums
         * happens once per pass, negligible next to the loop itself. */
        double s0 = 0.0, s1 = 0.0, s2 = 0.0, s3 = 0.0;
        double s4 = 0.0, s5 = 0.0, s6 = 0.0, s7 = 0.0;
        long i = 0;
        for (; i + 7 < n; i += 8) {
            s0 += buf[i];
            s1 += buf[i + 1];
            s2 += buf[i + 2];
            s3 += buf[i + 3];
            s4 += buf[i + 4];
            s5 += buf[i + 5];
            s6 += buf[i + 6];
            s7 += buf[i + 7];
        }
        double local = ((s0 + s1) + (s2 + s3)) + ((s4 + s5) + (s6 + s7));
        for (; i < n; i++) {
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

    bw_gate_t gate;
    bw_gate_init(&gate, nthreads);

    for (int i = 0; i < nthreads; i++) {
        ctxs[i].index = i;
        ctxs[i].opts = opts;
        ctxs[i].num_doubles = num_doubles;
        ctxs[i].gate = &gate;
        ctxs[i].target_passes = opts->iterations;
    }

    int create_failed = 0;
    int created = 0;
    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&tids[i], NULL, bw_worker, &ctxs[i]) != 0) {
            fprintf(stderr, "cyclelab: failed to start worker thread %d\n", i);
            create_failed = 1;
            break;
        }
        created++;
    }

    if (create_failed) {
        /* Wake any already-created workers out of bw_gate_worker_ready_and_wait
         * instead of letting them block forever on a ready count that can
         * now never reach `total` -- the deadlock the original barrier-based
         * version had on this exact path. */
        bw_gate_abort(&gate);
        for (int j = 0; j < created; j++) pthread_join(tids[j], NULL);
        for (int j = 0; j < nthreads; j++) free(ctxs[j].buf);
        bw_gate_destroy(&gate);
        free(ctxs);
        free(tids);
        return 1;
    }

    /* Blocks until every worker has finished allocating, then publishes
     * one shared start_time/deadline and releases all of them from the
     * same starting line -- a single common measurement window, instead
     * of each worker computing its own deadline from its own post-wakeup
     * timestamp, which can drift under scheduling jitter (especially
     * oversubscribed). */
    int aborted = bw_gate_main_release(&gate, opts->duration_s);
    double start_wall = gate.start_time;

    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }
    double duration_actual_s = aborted ? 0.0 : (timing_now_seconds() - start_wall);
    bw_gate_destroy(&gate);

    if (aborted) {
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
