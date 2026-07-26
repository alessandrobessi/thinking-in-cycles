# Part IV — Why Memory Changes Everything

Moves from CPU execution (Parts II-III) to the memory system: why memory
access can dominate code that does little computation (Chapter 16), how
access order and working-set size change performance independently of
each other (Chapter 17), how independent threads can contend through
the cache-coherence protocol without any logical data sharing (Chapter
18), how to tell whether a workload is bandwidth-bound rather than
latency-bound (Chapter 19), and a tool ladder for going beyond
controlled experiments to hardware-level memory observability (Chapter
20).

| Chapter | Title | Opening Question |
|---|---|---|
| 16 | [The Memory Hierarchy and Locality](chapter-16-the-memory-hierarchy-and-locality.md) | Why can memory access dominate code that performs little computation? |
| 17 | [Working Sets, Cache Misses, and Prefetching](chapter-17-working-sets-cache-misses-and-prefetching.md) | Why do access order and working-set size change performance? |
| 18 | [Cache Coherence and False Sharing](chapter-18-cache-coherence-and-false-sharing.md) | How can independent threads slow each other through cache coherence? |
| 19 | [Memory Bandwidth and the Roofline Intuition](chapter-19-memory-bandwidth-and-the-roofline-intuition.md) | How do we tell whether a workload is limited by memory bandwidth? |
| 20 | [Measuring Memory with `perf mem`, `perf c2c`, and PCM](chapter-20-measuring-memory-with-perf-mem-perf-c2c-and-pcm.md) | Which tools reveal cache and memory behavior? |

All five guided labs are **portable** — no root, no `perf`, no special
hardware — built on four new `labs/cyclelab` modes added in this phase:
`sequential-memory` and `random-memory` (a Sattolo's-algorithm
pointer-chase latency benchmark), `bandwidth` (a prefetch-friendly
streaming-sum kernel), and `false-sharing` (packed vs. padded per-thread
counters). Every chapter's numbers are real, captured data from this
book's reference machine (Apple M4, macOS, arm64) — genuine latency
cliffs, stride effects, coherence penalties, and bandwidth saturation
curves, not schematic illustrations. Chapter 20's tool ladder is the one
exception requiring Linux/`perf`/PCM for its higher rungs; its portable
lab instead synthesizes Chapters 16-19's own results as that ladder's
first, fully-tested rung.

Next: [Part V — Where the Work Runs](../part-5-where-the-work-runs/README.md) (Chapters 21-25).
