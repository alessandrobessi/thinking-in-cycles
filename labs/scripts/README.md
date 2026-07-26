# labs/scripts

Small helper scripts that back the Guided Lab section of each drafted
chapter. Each script is self-contained, checks its own prerequisites, and
prints an interpretation note rather than a bare number (BLUEPRINT.md
Section 9: "never require the reader to reproduce an exact numeric value").

All scripts that use `cyclelab` expect it to already be built:

```bash
make lab-cyclelab   # from the repo root
```

| Script | Chapter | What it does |
|---|---|---|
| `ch1_time_accounting.sh` | 1 | Compares wall/user/system time for a `cyclelab compute` run. |
| `ch2_size_sweep.sh` | 2 | Runs `cyclelab compute` at three iteration counts and two op mixes, tabulating throughput. |
| `ch3_concurrency_sweep.sh` | 3 | Drives a local `python3 -m http.server` at increasing concurrency (documented stand-in for the not-yet-built `labs/mini-service`). |
| `ch4_interleaved_ab.sh` | 4 | Runs two `cyclelab compute` configurations interleaved, for eyeballing distributions rather than single numbers. |
| `ch5_investigate_slow_config.sh` | 5 | Runs a deliberately over-threaded `cyclelab compute` config alongside `ps`/`vmstat` guidance, for working through the investigation loop. |
| `ch6_build_and_disassemble.sh` | 6 | Builds `cyclelab` at `-O0` and `-O2` and disassembles the same function from both, comparing static instruction count. |
| `ch7_ipc_intuition.sh` | 7 | Runs `cyclelab compute --chains=1` vs `--chains=8` (same instruction mix) as a portable, indirect view of an IPC difference. |
| `ch8_dependency_chains.sh` | 8 | Sweeps `cyclelab compute --chains` from 1 to 16, showing throughput rise-then-plateau as independent work saturates the pipeline. |
| `ch9_branch_prediction.sh` | 9 | Runs `cyclelab branch --pattern=sorted` vs `--pattern=random`, same conditional, to show a misprediction-driven throughput gap. |

Chapter 10 has no dedicated script: its lab is a direct `perf stat`
invocation documented in the chapter text (Linux-only; not testable on
this project's macOS reference machine), with Chapter 7's script serving
as its portable fallback.

| Script | Chapter | What it does |
|---|---|---|
| `capture_sample_profile.sh` | 11, 14, 15 | Reusable wrapper: runs a `cyclelab` command, profiles it with macOS `sample`, and folds the result into a `.folded` file. |
| `foldstacks.py` | 11-15 (tooling) | Converts macOS `sample` output into folded-stack format (`frame1;frame2 count`), the same format Linux's `stackcollapse-perf.pl` produces. |
| `flamegraph_svg.py` | 14, 15 (tooling) | Renders a folded-stack file to a static SVG flame graph; `--diff-against` enables Chapter 15's differential (red/blue) mode. |
| `ch12_profile_hot_path.sh` | 12 | Profiles `cyclelab compute --chains=1` and prints `sample`'s self-cost ranking plus a per-source-line breakdown (this book's `perf annotate` equivalent). |
| `ch13_symbol_availability.sh` | 13 | Builds full-debug, no-debug, and frame-pointer-omitted variants of the same source and compares what `sample` can reconstruct from each. |
| `ch15_before_after.sh` | 15 | Interleaved before/after throughput benchmark plus a differential flame graph, for `--chains=1` vs `--chains=8`. |

Chapters 11 and 14 use `capture_sample_profile.sh` (plus, for Chapter 14,
`flamegraph_svg.py`) directly rather than a dedicated per-chapter script.

| Script | Chapter | What it does |
|---|---|---|
| `ch16_memory_hierarchy.sh` | 16 | Sweeps `cyclelab random-memory --working-set-size` from 16K to 128M, showing the cache-hierarchy latency staircase. |
| `ch17_stride_sweep.sh` | 17 | Sweeps `cyclelab sequential-memory --stride` at a fixed 64MB working set, isolating access order from working-set size. |
| `ch18_false_sharing.sh` | 18 | Compares `cyclelab false-sharing --padding=packed` vs `padded` throughput across thread counts 1-10. |
| `ch19_bandwidth_scaling.sh` | 19 | Sweeps `cyclelab bandwidth --threads` at a fixed 64MB-per-thread working set, showing the aggregate bandwidth saturation curve. |

Chapter 20 has no dedicated script: its lab is a written synthesis
exercise over Chapters 16-19's own results, documented in the chapter
text (its `perf mem`/`perf c2c`/PCM commands are Linux/hardware-specific
and not testable on this project's macOS reference machine).

Every script above is **portable** (BLUEPRINT.md Section 13.2): no root,
no `perf`, no special hardware — except that the Chapter 11-15 scripts
specifically require macOS's built-in `sample`(1) utility, since this
project's reference machine has no `perf`. They run on Linux and macOS;
a few print platform-appropriate follow-up commands (e.g. `vmstat` vs.
`vm_stat`, or `objdump`'s macOS `--macho` flag vs. plain Linux usage)
rather than pretending one OS's tool exists everywhere.
