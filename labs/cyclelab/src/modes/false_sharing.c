#include "false_sharing.h"

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

/* Explicitly cache-line-aligned: struct size alone does not guarantee
 * that -- an unaligned base address would still let each 64-byte struct
 * straddle two cache lines, letting adjacent counters share one after
 * all. _Alignas plus an aligned allocation (aligned_alloc, below) are
 * both required for "no sharing possible" to actually hold. */
typedef struct {
    _Alignas(CYCLELAB_CACHE_LINE_BYTES) long counter;
    char pad[CYCLELAB_CACHE_LINE_BYTES - sizeof(long)];
} padded_counter_t;

_Static_assert(sizeof(padded_counter_t) == CYCLELAB_CACHE_LINE_BYTES,
               "padded_counter_t must be exactly one cache line");
_Static_assert(_Alignof(padded_counter_t) >= CYCLELAB_CACHE_LINE_BYTES,
               "padded_counter_t must be cache-line-aligned");

static const char *padding_name(cyclelab_padding_t p) {
    return (p == CYCLELAB_PADDING_PACKED) ? "packed" : "padded";
}

typedef struct {
    int index;
    const cyclelab_options_t *opts;
    volatile long *counter; /* points into whichever layout was built */
    double deadline;
    long target_increments;

    long increments_done;
    double elapsed_s;
    cyclelab_affinity_result_t affinity_result;
    const char *affinity_reason;
} fs_worker_ctx_t;

static void *fs_worker(void *arg) {
    fs_worker_ctx_t *ctx = (fs_worker_ctx_t *)arg;
    ctx->affinity_result = affinity_apply(ctx->opts->affinity_spec, ctx->index,
                                           ctx->opts->threads, &ctx->affinity_reason);

    volatile long *counter = ctx->counter;
    long done = 0;
    double start = timing_now_seconds();

    for (;;) {
        for (int u = 0; u < 64; u++) {
            *counter += 1;
        }
        done += 64;

        if (ctx->target_increments > 0) {
            if (done >= ctx->target_increments) break;
        } else {
            if ((done & 0xFFFF) == 0 && timing_now_seconds() >= ctx->deadline) break;
        }
    }

    ctx->elapsed_s = timing_now_seconds() - start;
    ctx->increments_done = done;
    return NULL;
}

static void print_json(FILE *out, const cyclelab_options_t *opts,
                        const cyclelab_hostinfo_t *host,
                        fs_worker_ctx_t *ctxs, int nthreads,
                        double duration_actual_s, long long total_increments,
                        long ctx_nvcsw, long ctx_nivcsw,
                        const char **warnings, int nwarnings) {
    char started_at[32];
    time_t now = time(NULL);
    struct tm tmv;
    gmtime_r(&now, &tmv);
    strftime(started_at, sizeof(started_at), "%Y-%m-%dT%H:%M:%SZ", &tmv);

    double throughput = (duration_actual_s > 0) ? ((double)total_increments / duration_actual_s) : 0.0;

    fprintf(out, "{\n");
    fprintf(out, "  \"tool\": \"cyclelab\",\n");
    fprintf(out, "  \"version\": \"%s\",\n", CYCLELAB_VERSION);
    fprintf(out, "  \"mode\": \"false-sharing\",\n");
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
        fprintf(out, "    \"increments_requested\": %ld,\n", opts->iterations);
    } else {
        fprintf(out, "    \"duration_requested_s\": %.6f,\n", opts->duration_s);
        fprintf(out, "    \"increments_requested\": null,\n");
    }
    fprintf(out, "    \"threads\": %d,\n", opts->threads);
    fprintf(out, "    \"affinity\": \""); json_write_escaped(out, opts->affinity_spec); fprintf(out, "\",\n");
    fprintf(out, "    \"padding\": \"%s\",\n", padding_name(opts->padding));
    fprintf(out, "    \"cache_line_bytes\": %d,\n", CYCLELAB_CACHE_LINE_BYTES);
    if (host->cache_line_bytes_detected > 0) {
        fprintf(out, "    \"cache_line_bytes_detected\": %d\n", host->cache_line_bytes_detected);
    } else {
        fprintf(out, "    \"cache_line_bytes_detected\": null\n");
    }
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
    fprintf(out, "    \"total_increments\": %lld,\n", total_increments);
    fprintf(out, "    \"throughput_increments_per_s\": %.2f,\n", throughput);
    fprintf(out, "    \"threads\": [\n");
    for (int i = 0; i < nthreads; i++) {
        fprintf(out, "      {\n");
        fprintf(out, "        \"index\": %d,\n", ctxs[i].index);
        fprintf(out, "        \"increments\": %ld,\n", ctxs[i].increments_done);
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
                        fs_worker_ctx_t *ctxs, int nthreads,
                        double duration_actual_s, long long total_increments,
                        long ctx_nvcsw, long ctx_nivcsw,
                        const char **warnings, int nwarnings) {
    double throughput = (duration_actual_s > 0) ? ((double)total_increments / duration_actual_s) : 0.0;

    fprintf(out, "cyclelab false-sharing -- %s/%s, %d logical CPUs, build=%s\n",
            host->os, host->arch, host->logical_cpus, CYCLELAB_BUILD_TYPE);
    fprintf(out, "threads=%d padding=%s\n", opts->threads, padding_name(opts->padding));
    for (int i = 0; i < nwarnings; i++) {
        fprintf(out, "warning: %s\n", warnings[i]);
    }
    fprintf(out, "duration_actual_s=%.6f total_increments=%lld throughput_increments_per_s=%.2f\n",
            duration_actual_s, total_increments, throughput);
    for (int i = 0; i < nthreads; i++) {
        fprintf(out, "  thread[%d] increments=%ld elapsed_s=%.6f affinity_applied=%s\n",
                ctxs[i].index, ctxs[i].increments_done, ctxs[i].elapsed_s,
                (ctxs[i].affinity_result == CYCLELAB_AFFINITY_APPLIED) ? "true" : "false");
    }
    fprintf(out, "context_switches voluntary=%ld involuntary=%ld\n", ctx_nvcsw, ctx_nivcsw);
}

int false_sharing_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host) {
    int nthreads = opts->threads;
    fs_worker_ctx_t *ctxs = calloc((size_t)nthreads, sizeof(fs_worker_ctx_t));
    pthread_t *tids = calloc((size_t)nthreads, sizeof(pthread_t));
    if (ctxs == NULL || tids == NULL) {
        fprintf(stderr, "cyclelab: out of memory allocating %d thread contexts\n", nthreads);
        free(ctxs);
        free(tids);
        return 1;
    }

    /* Packed: one plain array, several counters per cache line.
     * Padded: one cache line per counter, no sharing possible. */
    volatile long *packed = NULL;
    padded_counter_t *padded = NULL;
    if (opts->padding == CYCLELAB_PADDING_PACKED) {
        packed = calloc((size_t)nthreads, sizeof(long));
    } else {
        /* aligned_alloc, not calloc/malloc: a plain allocator gives no
         * guarantee the returned address is a multiple of the cache line
         * size, and without that guarantee two adjacent 64-byte structs
         * can still straddle a shared line -- see padded_counter_t above. */
        size_t padded_bytes = (size_t)nthreads * sizeof(padded_counter_t);
        padded = aligned_alloc(CYCLELAB_CACHE_LINE_BYTES, padded_bytes);
        if (padded != NULL) memset(padded, 0, padded_bytes);
    }
    if ((opts->padding == CYCLELAB_PADDING_PACKED && packed == NULL) ||
        (opts->padding == CYCLELAB_PADDING_PADDED && padded == NULL)) {
        fprintf(stderr, "cyclelab: out of memory allocating counters\n");
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
        ctxs[i].target_increments = opts->iterations;
        ctxs[i].counter = (opts->padding == CYCLELAB_PADDING_PACKED)
            ? &packed[i]
            : &padded[i].counter;
    }

    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&tids[i], NULL, fs_worker, &ctxs[i]) != 0) {
            fprintf(stderr, "cyclelab: failed to start worker thread %d\n", i);
            for (int j = 0; j < i; j++) pthread_join(tids[j], NULL);
            free(ctxs);
            free(tids);
            free((void *)packed);
            free(padded);
            return 1;
        }
    }
    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }
    double duration_actual_s = timing_now_seconds() - start_wall;

    long long total_increments = 0;
    for (int i = 0; i < nthreads; i++) {
        total_increments += ctxs[i].increments_done;
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
    /* padded_counter_t's alignment guarantee is only as good as
     * CYCLELAB_CACHE_LINE_BYTES actually matching this machine's real
     * coherence granule. A smaller detected value is harmless (padding
     * to a larger size than strictly necessary never causes sharing);
     * a *larger* one would mean two "padded" counters could still land
     * on the same coherence line, quietly invalidating "cannot share" --
     * so only the larger case is worth a warning. */
    char cache_line_warning[160];
    if (opts->padding == CYCLELAB_PADDING_PADDED &&
        host->cache_line_bytes_detected > CYCLELAB_CACHE_LINE_BYTES &&
        nwarnings < 64) {
        snprintf(cache_line_warning, sizeof(cache_line_warning),
                 "detected cache line size (%d bytes) exceeds this build's "
                 "compiled-in assumption (%d bytes); the padded layout's "
                 "no-sharing guarantee does not hold on this machine",
                 host->cache_line_bytes_detected, CYCLELAB_CACHE_LINE_BYTES);
        warnings[nwarnings++] = cache_line_warning;
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
            free((void *)packed);
            free(padded);
            return 1;
        }
        close_out = 1;
    }

    long ctx_nvcsw, ctx_nivcsw;
    rusage_get_context_switches(&ctx_nvcsw, &ctx_nivcsw);

    if (opts->format == CYCLELAB_FMT_JSON) {
        print_json(out, opts, host, ctxs, nthreads, duration_actual_s, total_increments,
                   ctx_nvcsw, ctx_nivcsw, warnings, nwarnings);
    } else {
        print_text(out, opts, host, ctxs, nthreads, duration_actual_s, total_increments,
                   ctx_nvcsw, ctx_nivcsw, warnings, nwarnings);
    }

    if (close_out) fclose(out);
    free(ctxs);
    free(tids);
    free((void *)packed);
    free(padded);
    return 0;
}
