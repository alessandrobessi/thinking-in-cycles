# cyclelab

`cyclelab` is the recurring command-line workload generator for *Thinking in
Cycles* (BLUEPRINT.md Section 8). It exposes different performance
phenomena through simple, configurable modes rather than requiring a new
example codebase per chapter.

## Mode status

| Mode | Status |
|---|---|
| `compute` | implemented |
| `branch` | implemented |
| `sequential-memory` | implemented |
| `random-memory` | implemented |
| `bandwidth` | implemented |
| `false-sharing` | implemented |
| `lock-contention` | not yet implemented |
| `syscall` | not yet implemented |
| `sleep` | not yet implemented |
| `numa` | not yet implemented |
| `mixed` | not yet implemented |

Unimplemented modes are still recognized by the CLI: they print
`"<mode>: not yet implemented"` to stderr and exit with code `2`, rather
than being rejected as unknown. This keeps the full CLI surface stable
across the book (Section 8), and Chapter 1's guided lab deliberately runs
one to show the reader what's coming.

## Build

```bash
make debug    # bin/cyclelab-debug  -- -O0 -g, for teaching/stepping
make release  # bin/cyclelab        -- -O2 -g, debug symbols retained
make          # both
make clean
```

Requires a C11 compiler and pthreads; builds on Linux (x86-64/Arm64) and
macOS (Arm64/x86-64).

## Usage

```text
cyclelab <mode> [options]

global options:
  --duration=SEC        time-boxed run length (default 2.0)
  --iterations=N        fixed per-thread iteration count (overrides --duration)
  --threads=N           worker thread count (default 1)
  --affinity=SPEC       none (default) | spread | comma-separated CPU list
  --seed=N              PRNG seed (default: time+pid derived)
  --format=json|text    output format (default json)
  --output=PATH|-       output destination (default "-", i.e. stdout)
  --quiet               suppress warnings on stderr
  --help, --version

compute-specific options:
  --op=int|float|mixed  which accumulator chain to run (default mixed)
  --chains=N            independent accumulator chains per thread, 1-16 (default 1)

branch-specific options:
  --pattern=sorted|random   table order to walk (default sorted)
  --branch-table-size=N     per-thread table size (default 1000000)

sequential-memory/random-memory-specific options:
  --working-set-size=BYTES  per-thread buffer size, K/M/G suffixes ok (default 1M)
  --pattern=sequential|random  overrides the mode's default access order
  --stride=N                slots (64B each) advanced per step, sequential only

bandwidth-specific options:
  --working-set-size=BYTES  per-thread buffer size, K/M/G suffixes ok (default 1M;
                            use larger than your last-level cache for real DRAM bandwidth)

false-sharing-specific options:
  --padding=packed|padded   counter layout (default packed)
```

Examples:

```bash
./bin/cyclelab compute --duration=2 --threads=4
./bin/cyclelab compute --iterations=5000000 --op=float --format=text
./bin/cyclelab compute --duration=1 --chains=8       # independent-chain ILP demo
./bin/cyclelab branch --pattern=sorted --duration=1
./bin/cyclelab branch --pattern=random --duration=1  # compare against the above
./bin/cyclelab random-memory --working-set-size=64M --duration=1   # cache/DRAM latency
./bin/cyclelab sequential-memory --working-set-size=64M --duration=1  # same size, prefetch-friendly
./bin/cyclelab bandwidth --working-set-size=128M --threads=8 --duration=1  # sustained GB/s
./bin/cyclelab false-sharing --padding=packed --threads=8 --duration=1
./bin/cyclelab false-sharing --padding=padded --threads=8 --duration=1  # compare against the above
./bin/cyclelab sleep          # not yet implemented -> exit 2
```

`--affinity` is best-effort: on Linux it uses `pthread_setaffinity_np`; on
macOS (and any other non-Linux target) there is no portable, unprivileged
hard-affinity API, so cyclelab prints a warning to stderr and continues
without pinning rather than failing (BLUEPRINT.md Section 8: "gracefully
reports unavailable features"). Pass `--quiet` to suppress the warning.

## `compute` mode

Each of `--threads` worker threads runs a tight loop with a genuine,
loop-carried dependency between an integer accumulator and a double
accumulator (`--op=int`, `--op=float`, or both interleaved via
`--op=mixed`), seeded from `--seed` and the thread index. The loop-carried
dependency, plus `volatile` accumulators and printing every thread's final
checksum, exists specifically so the compiler cannot fold the work away
(BLUEPRINT.md Section 8: "prints work completed and a checksum to prevent
dead-code elimination").

A run is time-boxed by `--duration` (checked every 1024 iterations to keep
timing overhead negligible) unless `--iterations` is given, which runs
each thread for exactly that many iterations instead.

`--chains=N` (default 1) splits each thread's work across N independent
accumulator chains instead of one: with `--chains=1`, unroll slot *u*
always updates the same accumulator, so slot *u+1* must wait for slot
*u*'s result; with `--chains=8`, the 8 unrolled slots each update their
*own* chain, so none of them depend on each other within an iteration.
This is what Chapter 8 uses to make the CPU's available instruction-level
parallelism visible as a throughput difference rather than an abstract
claim -- on the reference machine for this book, `--chains=8` measured
roughly 4x the throughput of `--chains=1` for `--op=int`.

## `branch` mode

Each of `--threads` worker threads builds its own `--branch-table-size`
table of pseudo-random byte values (deterministic from `--seed`, via a
small built-in xorshift64 generator -- not libc's `rand()`, so table
contents are reproducible across platforms), in either `--pattern=sorted`
or `--pattern=random` order, then repeatedly walks the table applying a
data-dependent conditional (`if (value >= 128) ... else ...`). Sorted
order groups long runs of the same branch outcome together, which a
branch predictor learns easily; random order does not. On the reference
machine for this book, `--pattern=sorted` measured roughly 3x the
throughput of `--pattern=random` at the same table size -- entirely from
how predictable the same conditional was, with identical work otherwise.

One "table pass" is one full walk of the table; `--iterations=N` (if
given) runs each thread for exactly N full passes instead of time-boxing
by `--duration`.

## `sequential-memory` / `random-memory` modes

Each of `--threads` worker threads builds its own `--working-set-size`
buffer, divided into 64-byte slots, then repeatedly chases a pointer
through it (`buf[cur].next`) -- a genuine dependent-load chain, so
elapsed time per step is a real latency measurement, not something
out-of-order execution or prefetching can hide. `sequential-memory`
defaults to `--pattern=sequential` (advancing `--stride` slots each
step, wrapping); `random-memory` defaults to `--pattern=random` (a
single-cycle random permutation of every slot, built with Sattolo's
algorithm so the whole buffer is genuinely exercised, not a handful of
short, independent sub-cycles). Either mode accepts either pattern via
`--pattern`. On the reference machine for this book, sweeping
`--working-set-size` with `random-memory` showed clean latency cliffs:
~1.6-1.8ns/access up to 128K, ~5.5-9.9ns/access from 256K-8M, and
~30-106ns/access from 16M-128M -- roughly L1/L2, LLC, and DRAM. At a
fixed 64M working set, `sequential-memory` measured ~2.1ns/access versus
`random-memory`'s ~93.8ns/access -- prefetching hiding almost all of the
DRAM latency for the predictable pattern and none of it for the random
one.

## `bandwidth` mode

Each of `--threads` worker threads streams sequentially through its own
`--working-set-size` buffer of doubles, summing every element in a
plain, non-dependent loop the compiler can vectorize and the CPU can
prefetch aggressively -- the opposite access shape from
sequential-memory/random-memory's deliberately dependent pointer chase,
because bandwidth measurement needs the hardware's latency-hiding
tricks turned on, not defeated. Use a `--working-set-size` well beyond
your machine's last-level cache to measure real DRAM bandwidth rather
than cache bandwidth. On the reference machine for this book, sweeping
`--threads` at a fixed 64M working set showed clear saturation:
throughput scaled roughly linearly from 1 to 4 threads (14.6 to 52.3
GB/s) and then flattened from 6 to 10 threads (65.6 to 76.2 GB/s) as the
shared memory channels approached their sustainable limit.

## `false-sharing` mode

Each of `--threads` worker threads repeatedly increments its own
dedicated counter, as fast as possible, with no lock and (in source-code
terms) no reason to interact with any other thread. In
`--padding=packed` (default), the counters sit in one tightly packed
array, so several typically share a 64-byte cache line; in
`--padding=padded`, each counter is padded out to its own exclusive
cache line. Any scaling difference between the two layouts at the same
thread count is false sharing -- cache-coherence traffic from threads
writing to the same line, even though each only touches its own
logically distinct counter. On the reference machine for this book,
`--padding=padded` measured consistently higher throughput than
`--padding=packed` at every thread count tested, growing to roughly 46%
higher at 8 threads.

## Output schema (stable core, extended by later modes)

```jsonc
{
  "tool": "cyclelab",
  "version": "0.1.0",
  "mode": "compute",
  "started_at": "2026-01-01T00:00:00Z",
  "build":  { "type": "release", "cflags": "...", "compiler": "..." },
  "host":   { "os": "...", "kernel": "...", "arch": "...", "hostname": "...", "logical_cpus": 8 },
  "config": { "duration_requested_s": 2.0, "iterations_requested": null,
              "threads": 4, "affinity": "none", "seed": 12345, "op": "mixed",
              "chains": 1 },
  "warnings": ["..."],
  "results": {
    "duration_actual_s": 2.000123,
    "total_iterations": 123456789,
    "throughput_ops_per_s": 493826000.5,
    "threads": [
      { "index": 0, "iterations": 30864000, "elapsed_s": 2.0001,
        "affinity_applied": false, "checksum": "<32 hex chars>" }
    ],
    "combined_checksum": "<16 hex chars>"
  }
}
```

`branch` mode uses the same `tool`/`version`/`mode`/`started_at`/`build`/
`host`/`warnings` envelope, but its own `config` and `results` shape,
since it measures a different thing (table passes and elements
processed, not iterations):

```jsonc
{
  "mode": "branch",
  "config": { "duration_requested_s": 2.0, "table_passes_requested": null,
              "threads": 4, "affinity": "none", "seed": 12345,
              "pattern": "sorted", "branch_table_size": 1000000 },
  "results": {
    "duration_actual_s": 2.000123,
    "total_table_passes": 1234,
    "total_elements_processed": 1234000000,
    "throughput_elements_per_s": 616950000.0,
    "threads": [
      { "index": 0, "table_passes": 308, "elapsed_s": 2.0001,
        "affinity_applied": false, "checksum": "<16 hex chars>" }
    ],
    "combined_checksum": "<16 hex chars>"
  }
}
```

`sequential-memory`/`random-memory` share one schema (`mode` differs):

```jsonc
{
  "mode": "random-memory",
  "config": { "duration_requested_s": 2.0, "steps_requested": null,
              "threads": 1, "affinity": "none", "seed": 12345,
              "pattern": "random", "working_set_bytes": 67108864,
              "num_slots": 1048576, "cache_line_bytes": 64, "stride_slots": 1 },
  "results": {
    "duration_actual_s": 2.000123,
    "total_steps": 21312,
    "ns_per_access": 93.841,
    "threads": [
      { "index": 0, "steps": 21312, "elapsed_s": 2.0001,
        "affinity_applied": false, "checksum": 512034 }
    ]
  }
}
```

`bandwidth` mode:

```jsonc
{
  "mode": "bandwidth",
  "config": { "duration_requested_s": 2.0, "passes_requested": null,
              "threads": 8, "affinity": "none", "working_set_bytes": 67108864 },
  "results": {
    "duration_actual_s": 2.000123,
    "total_passes": 2456,
    "total_bytes_read": 164698112000,
    "bandwidth_gb_per_s": 74.29,
    "threads": [
      { "index": 0, "passes": 307, "elapsed_s": 2.0001, "affinity_applied": false }
    ]
  }
}
```

`false-sharing` mode:

```jsonc
{
  "mode": "false-sharing",
  "config": { "duration_requested_s": 2.0, "increments_requested": null,
              "threads": 8, "affinity": "none", "padding": "padded",
              "cache_line_bytes": 64 },
  "results": {
    "duration_actual_s": 2.000123,
    "total_increments": 12290604610,
    "throughput_increments_per_s": 6145302305.0,
    "threads": [
      { "index": 0, "increments": 1536325576, "elapsed_s": 2.0001, "affinity_applied": false }
    ]
  }
}
```

`--format=text` prints the same information as human-readable lines
instead of JSON, for all modes. Every field the book's guided labs read
from `--format=json` output is considered part of that mode's stable
core and will only ever gain siblings, not change meaning, as later
modes are added.

## Exit codes

| Code | Meaning |
|---|---|
| 0 | success |
| 1 | internal error (e.g. output file could not be opened) |
| 2 | recognized mode, not yet implemented |
| 64 | usage error (matches BSD `sysexits.h` `EX_USAGE`) |
