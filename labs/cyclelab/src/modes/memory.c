#include "memory.h"

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

typedef struct {
    long next;
    char pad[CYCLELAB_CACHE_LINE_BYTES - sizeof(long)];
} mem_slot_t;

typedef struct {
    int index;
    const cyclelab_options_t *opts;
    mem_slot_t *buf;
    long num_slots;
    double deadline;
    long target_steps;

    long steps_done;
    double elapsed_s;
    long checksum;
    cyclelab_affinity_result_t affinity_result;
    const char *affinity_reason;
} memory_worker_ctx_t;

static const char *pattern_name(cyclelab_pattern_t p) {
    return (p == CYCLELAB_PATTERN_SORTED) ? "sequential" : "random";
}

static uint64_t xorshift64(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

/* Builds one thread's pointer-chase buffer. Sequential order advances
 * --stride slots each step (wrapping); random order is a single-cycle
 * permutation via Sattolo's algorithm. */
static mem_slot_t *build_buffer(long working_set_bytes, cyclelab_pattern_t pattern,
                                 long stride, unsigned long seed, long *out_num_slots) {
    long num_slots = working_set_bytes / (long)sizeof(mem_slot_t);
    if (num_slots < 2) num_slots = 2;

    mem_slot_t *buf = malloc((size_t)num_slots * sizeof(mem_slot_t));
    if (buf == NULL) return NULL;

    if (pattern == CYCLELAB_PATTERN_RANDOM) {
        long *perm = malloc((size_t)num_slots * sizeof(long));
        if (perm == NULL) {
            free(buf);
            return NULL;
        }
        for (long i = 0; i < num_slots; i++) perm[i] = i;

        uint64_t state = seed ? (uint64_t)seed : 1;
        for (long i = num_slots - 1; i >= 1; i--) {
            uint64_t r = xorshift64(&state);
            long j = (long)(r % (uint64_t)i); /* Sattolo: j in [0, i-1], not [0, i] */
            long tmp = perm[i];
            perm[i] = perm[j];
            perm[j] = tmp;
        }
        for (long i = 0; i < num_slots; i++) buf[i].next = perm[i];
        free(perm);
    } else {
        for (long i = 0; i < num_slots; i++) {
            buf[i].next = (i + stride) % num_slots;
        }
    }

    *out_num_slots = num_slots;
    return buf;
}

static void *memory_worker(void *arg) {
    memory_worker_ctx_t *ctx = (memory_worker_ctx_t *)arg;
    ctx->affinity_result = affinity_apply(ctx->opts->affinity_spec, ctx->index,
                                           ctx->opts->threads, &ctx->affinity_reason);

    /* The dependent load (each step needs the previous step's result to
     * know which address to read next) is what makes this a genuine
     * latency measurement rather than something out-of-order execution
     * or hardware prefetching can hide -- the same principle Chapter 8
     * used deliberately, here applied to memory instead of arithmetic. */
    volatile long cur = 0;
    mem_slot_t *buf = ctx->buf;
    long steps = 0;
    double start = timing_now_seconds();

    for (;;) {
        for (int u = 0; u < 8; u++) {
            cur = buf[cur].next;
        }
        steps += 8;

        if (ctx->target_steps > 0) {
            if (steps >= ctx->target_steps) break;
        } else {
            if ((steps & 0x1FFF) == 0 && timing_now_seconds() >= ctx->deadline) break;
        }
    }

    ctx->elapsed_s = timing_now_seconds() - start;
    ctx->steps_done = steps;
    ctx->checksum = cur;
    return NULL;
}

static void print_json(FILE *out, const cyclelab_options_t *opts,
                        const cyclelab_hostinfo_t *host,
                        memory_worker_ctx_t *ctxs, int nthreads,
                        double duration_actual_s, long long total_steps,
                        const char **warnings, int nwarnings) {
    char started_at[32];
    time_t now = time(NULL);
    struct tm tmv;
    gmtime_r(&now, &tmv);
    strftime(started_at, sizeof(started_at), "%Y-%m-%dT%H:%M:%SZ", &tmv);

    double ns_per_access = (total_steps > 0)
        ? (duration_actual_s * 1e9 * (double)nthreads / (double)total_steps)
        : 0.0;

    fprintf(out, "{\n");
    fprintf(out, "  \"tool\": \"cyclelab\",\n");
    fprintf(out, "  \"version\": \"%s\",\n", CYCLELAB_VERSION);
    fprintf(out, "  \"mode\": \"%s\",\n", opts->mode);
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
        fprintf(out, "    \"steps_requested\": %ld,\n", opts->iterations);
    } else {
        fprintf(out, "    \"duration_requested_s\": %.6f,\n", opts->duration_s);
        fprintf(out, "    \"steps_requested\": null,\n");
    }
    fprintf(out, "    \"threads\": %d,\n", opts->threads);
    fprintf(out, "    \"affinity\": \""); json_write_escaped(out, opts->affinity_spec); fprintf(out, "\",\n");
    fprintf(out, "    \"seed\": %lu,\n", opts->seed);
    fprintf(out, "    \"pattern\": \"%s\",\n", pattern_name(opts->pattern));
    fprintf(out, "    \"working_set_bytes\": %ld,\n", opts->working_set_bytes);
    fprintf(out, "    \"num_slots\": %ld,\n", ctxs[0].num_slots);
    fprintf(out, "    \"cache_line_bytes\": %d,\n", CYCLELAB_CACHE_LINE_BYTES);
    fprintf(out, "    \"stride_slots\": %ld\n", opts->mem_stride_slots);
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
    fprintf(out, "    \"total_steps\": %lld,\n", total_steps);
    fprintf(out, "    \"ns_per_access\": %.3f,\n", ns_per_access);
    fprintf(out, "    \"threads\": [\n");
    for (int i = 0; i < nthreads; i++) {
        fprintf(out, "      {\n");
        fprintf(out, "        \"index\": %d,\n", ctxs[i].index);
        fprintf(out, "        \"steps\": %ld,\n", ctxs[i].steps_done);
        fprintf(out, "        \"elapsed_s\": %.6f,\n", ctxs[i].elapsed_s);
        fprintf(out, "        \"affinity_applied\": %s,\n",
                (ctxs[i].affinity_result == CYCLELAB_AFFINITY_APPLIED) ? "true" : "false");
        fprintf(out, "        \"checksum\": %ld\n", ctxs[i].checksum);
        fprintf(out, "      }%s\n", (i == nthreads - 1) ? "" : ",");
    }
    fprintf(out, "    ]\n");
    fprintf(out, "  }\n");
    fprintf(out, "}\n");
}

static void print_text(FILE *out, const cyclelab_options_t *opts,
                        const cyclelab_hostinfo_t *host,
                        memory_worker_ctx_t *ctxs, int nthreads,
                        double duration_actual_s, long long total_steps,
                        const char **warnings, int nwarnings) {
    double ns_per_access = (total_steps > 0)
        ? (duration_actual_s * 1e9 * (double)nthreads / (double)total_steps)
        : 0.0;

    fprintf(out, "cyclelab %s -- %s/%s, %d logical CPUs, build=%s\n",
            opts->mode, host->os, host->arch, host->logical_cpus, CYCLELAB_BUILD_TYPE);
    fprintf(out, "threads=%d pattern=%s working_set_bytes=%ld num_slots=%ld stride=%ld\n",
            opts->threads, pattern_name(opts->pattern), opts->working_set_bytes,
            ctxs[0].num_slots, opts->mem_stride_slots);
    for (int i = 0; i < nwarnings; i++) {
        fprintf(out, "warning: %s\n", warnings[i]);
    }
    fprintf(out, "duration_actual_s=%.6f total_steps=%lld ns_per_access=%.3f\n",
            duration_actual_s, total_steps, ns_per_access);
    for (int i = 0; i < nthreads; i++) {
        fprintf(out, "  thread[%d] steps=%ld elapsed_s=%.6f affinity_applied=%s\n",
                ctxs[i].index, ctxs[i].steps_done, ctxs[i].elapsed_s,
                (ctxs[i].affinity_result == CYCLELAB_AFFINITY_APPLIED) ? "true" : "false");
    }
}

int memory_run(const cyclelab_options_t *opts, const cyclelab_hostinfo_t *host) {
    int nthreads = opts->threads;
    memory_worker_ctx_t *ctxs = calloc((size_t)nthreads, sizeof(memory_worker_ctx_t));
    pthread_t *tids = calloc((size_t)nthreads, sizeof(pthread_t));
    if (ctxs == NULL || tids == NULL) {
        fprintf(stderr, "cyclelab: out of memory allocating %d thread contexts\n", nthreads);
        free(ctxs);
        free(tids);
        return 1;
    }

    for (int i = 0; i < nthreads; i++) {
        unsigned long buf_seed = opts->seed + (unsigned long)i * 2654435761UL;
        ctxs[i].buf = build_buffer(opts->working_set_bytes, opts->pattern,
                                    opts->mem_stride_slots, buf_seed, &ctxs[i].num_slots);
        if (ctxs[i].buf == NULL) {
            fprintf(stderr, "cyclelab: out of memory building a %ld-byte buffer for thread %d\n",
                    opts->working_set_bytes, i);
            for (int j = 0; j < i; j++) free(ctxs[j].buf);
            free(ctxs);
            free(tids);
            return 1;
        }
    }

    double start_wall = timing_now_seconds();
    double deadline = start_wall + opts->duration_s;

    for (int i = 0; i < nthreads; i++) {
        ctxs[i].index = i;
        ctxs[i].opts = opts;
        ctxs[i].deadline = deadline;
        ctxs[i].target_steps = opts->iterations;
    }

    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&tids[i], NULL, memory_worker, &ctxs[i]) != 0) {
            fprintf(stderr, "cyclelab: failed to start worker thread %d\n", i);
            for (int j = 0; j < i; j++) pthread_join(tids[j], NULL);
            for (int j = 0; j < nthreads; j++) free(ctxs[j].buf);
            free(ctxs);
            free(tids);
            return 1;
        }
    }
    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }
    double duration_actual_s = timing_now_seconds() - start_wall;

    long long total_steps = 0;
    for (int i = 0; i < nthreads; i++) {
        total_steps += ctxs[i].steps_done;
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

    if (opts->format == CYCLELAB_FMT_JSON) {
        print_json(out, opts, host, ctxs, nthreads, duration_actual_s, total_steps, warnings, nwarnings);
    } else {
        print_text(out, opts, host, ctxs, nthreads, duration_actual_s, total_steps, warnings, nwarnings);
    }

    if (close_out) fclose(out);
    for (int i = 0; i < nthreads; i++) free(ctxs[i].buf);
    free(ctxs);
    free(tids);
    return 0;
}
