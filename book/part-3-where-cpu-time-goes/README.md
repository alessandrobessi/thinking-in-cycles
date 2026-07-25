# Part III — Where the CPU Time Goes

Moves from counting and measuring cycles (Part II) to locating *which*
code paths consume them: choosing between counting, sampling, and
tracing as observation models (Chapter 11), reading a CPU profile's
self versus inclusive cost correctly (Chapter 12), understanding why a
profiler's call stacks are only as good as the symbols and unwinding
metadata available to it (Chapter 13), reading flame graphs as a shape
rather than a timeline (Chapter 14), and proving an optimization
actually worked rather than just looking like it did (Chapter 15).

| Chapter | Title | Opening Question |
|---|---|---|
| 11 | [Counting, Sampling, and Tracing](chapter-11-counting-sampling-and-tracing.md) | When should we count, sample, or trace? |
| 12 | [`perf record`, `perf report`, and `perf annotate`](chapter-12-perf-record-perf-report-and-perf-annotate.md) | Which functions and code paths consume CPU time? |
| 13 | [Stacks, Symbols, and Unwinding](chapter-13-stacks-symbols-and-unwinding.md) | Why are call stacks sometimes missing or wrong? |
| 14 | [Flame Graphs: Reading the Shape of Work](chapter-14-flame-graphs-reading-the-shape-of-work.md) | How do flame graphs show the shape of work? |
| 15 | [Differential Profiling and Optimization Proof](chapter-15-differential-profiling-and-optimization-proof.md) | How do we prove that an optimization changed the right thing? |

This Part's guided labs are all **portable**, built on macOS's built-in
`sample`(1) utility rather than `perf` (unavailable on this book's
macOS reference machine) — a genuine, tested capture-report-annotate
workflow, not a schematic stand-in. Linux `perf`/`perf record`/`perf
report`/`perf annotate`/`perf diff` commands are documented throughout
as the standard equivalent, clearly marked as untested here.

This Part also introduces new, purpose-built tooling beyond `cyclelab`
itself:

- `labs/scripts/foldstacks.py` — converts `sample` output into the same
  folded-stack format Linux's `stackcollapse-perf.pl` produces.
- `labs/scripts/flamegraph_svg.py` — a minimal, dependency-free
  folded-stack-to-SVG flame graph renderer, including a differential
  (`--diff-against`) mode for Chapter 15.
- `labs/scripts/capture_sample_profile.sh` — a reusable capture+fold
  wrapper used by Chapters 11, 12, 14, and 15's labs.

Chapter 14 also includes this Part's one real figure
(`figures/generated/ch14-flame-graph-example.svg`) — a deliberate,
narrow exception to the "no figures yet" policy from Parts I-II, since
Chapter 14 is specifically about reading a visual artifact and a real,
captured one was available to use instead of a schematic placeholder.

Next: Part IV — Why Memory Changes Everything (Chapters 16-20, not yet
drafted).
