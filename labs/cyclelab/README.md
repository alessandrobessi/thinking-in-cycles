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
| `sequential-memory` | not yet implemented |
| `random-memory` | not yet implemented |
| `bandwidth` | not yet implemented |
| `false-sharing` | not yet implemented |
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
```

Examples:

```bash
./bin/cyclelab compute --duration=2 --threads=4
./bin/cyclelab compute --iterations=5000000 --op=float --format=text
./bin/cyclelab compute --duration=1 --chains=8       # independent-chain ILP demo
./bin/cyclelab branch --pattern=sorted --duration=1
./bin/cyclelab branch --pattern=random --duration=1  # compare against the above
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

`--format=text` prints the same information as human-readable lines
instead of JSON, for both modes. Every field the book's guided labs read
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
