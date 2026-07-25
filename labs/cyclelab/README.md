# cyclelab

`cyclelab` is the recurring command-line workload generator for *Thinking in
Cycles* (BLUEPRINT.md Section 8). It exposes different performance
phenomena through simple, configurable modes rather than requiring a new
example codebase per chapter.

## Mode status

| Mode | Status |
|---|---|
| `compute` | implemented |
| `branch` | not yet implemented (see `ROADMAP.md`) |
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
```

Examples:

```bash
./bin/cyclelab compute --duration=2 --threads=4
./bin/cyclelab compute --iterations=5000000 --op=float --format=text
./bin/cyclelab branch          # not yet implemented -> exit 2
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
              "threads": 4, "affinity": "none", "seed": 12345, "op": "mixed" },
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

`--format=text` prints the same information as human-readable lines instead
of JSON. Every field the book's guided labs read from `--format=json`
output (`results.total_iterations`, `results.throughput_ops_per_s`,
`results.combined_checksum`) is considered part of the stable core and
will only ever gain siblings, not change meaning, as later modes are added.

## Exit codes

| Code | Meaning |
|---|---|
| 0 | success |
| 1 | internal error (e.g. output file could not be opened) |
| 2 | recognized mode, not yet implemented |
| 64 | usage error (matches BSD `sysexits.h` `EX_USAGE`) |
