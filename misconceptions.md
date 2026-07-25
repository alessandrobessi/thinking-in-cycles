# Misconception Registry

Seed table reproduced verbatim from BLUEPRINT.md Section 16, then expanded
into full entries per BLUEPRINT.md Section 9's "Common Misconceptions"
requirement (why it's wrong / correct intuition / distinguishing evidence)
and tracked by which chapter actually uses each one. M21–M23 are new
entries proposed while drafting Chapters 2, 3, and 5, where none of the
seed M01–M20 fit well — each is marked **proposed — pending review** since
the Section 16 seed didn't anticipate them.

## Seed table (BLUEPRINT.md Section 16, verbatim)

| ID | Misconception | Correct intuition |
|---|---|---|
| M01 | Low average CPU usage means the CPU cannot be involved in latency. | Critical threads can wait runnable, serialize, migrate, or use only one core while the machine average remains low. |
| M02 | 100% CPU means a process is CPU-bound in the useful-work sense. | A CPU can be busy retiring waste, spinning, handling kernel work, or waiting on memory while still reporting busy time. |
| M03 | Fewer instructions always means faster code. | Instruction count is one factor; stalls, vector width, memory behavior, and frequency also matter. |
| M04 | Higher IPC always means better performance. | IPC describes pipeline utilization for a workload; elapsed time and completed work remain primary. |
| M05 | A high cache-miss percentage proves a cache bottleneck. | Rates need access volume, miss cost, overlap, and workload context. |
| M06 | A flame graph is a timeline. | It aggregates stack samples; horizontal position is not chronological. |
| M07 | The widest frame is the function to optimize. | Width may represent necessary work, a wrapper, or accumulated child cost. |
| M08 | Sampling profiles show all latency. | They primarily show on-CPU execution unless an off-CPU method is used. |
| M09 | Pinning always improves performance. | Affinity can reduce migrations or increase queueing and imbalance. |
| M10 | CPU affinity also binds memory. | CPU and memory placement are separate policies. |
| M11 | NUMA matters only at enormous scale. | Any multi-node system can suffer remote-memory cost and bandwidth imbalance. |
| M12 | Local memory is always optimal. | Interleaving can improve aggregate bandwidth; placement depends on access pattern. |
| M13 | eBPF has zero overhead. | Overhead depends on hook rate, work per event, stack capture, aggregation, and output. |
| M14 | More tracing produces more truth. | Excess event volume can perturb the workload and bury the useful signal. |
| M15 | One benchmark run is evidence. | One run is an anecdote unless the effect is overwhelming and the environment is controlled. |
| M16 | A microbenchmark improvement guarantees a production improvement. | Production may have a different workload, bottleneck, concurrency pattern, or critical path. |
| M17 | An optimization is complete when the original hotspot shrinks. | The workload outcome must improve, and the bottleneck may move elsewhere. |
| M18 | Vendor peak bandwidth is the expected application bandwidth. | Sustainable bandwidth depends on channels, access pattern, concurrency, instructions, and platform configuration. |
| M19 | Context-switch counts alone diagnose scheduler overhead. | The impact depends on why switches occur, where the critical thread waits, and what locality is lost. |
| M20 | A profiler's output is ground truth. | Every profiler is a measurement system with scope, overhead, permissions, and blind spots. |

## Full registry

### M01
**Misconception:** Low average CPU usage means the CPU cannot be involved in latency.
**Correct intuition:** Critical threads can wait runnable, serialize, migrate, or use only one core while the machine average remains low.
**Evidence that distinguishes:** Compare machine-wide CPU utilization against the wall time of the single critical request path; a low average can coexist with one saturated core or one serialized thread on the request's critical path.
**Used in chapters:** 1

### M02
**Misconception:** 100% CPU means a process is CPU-bound in the useful-work sense.
**Correct intuition:** A CPU can be busy retiring waste, spinning, handling kernel work, or waiting on memory while still reporting busy time.
**Evidence that distinguishes:** Compare user vs. system time, and completed work per CPU-second, across two runs with the same "100% busy" reading — busy time alone does not certify useful work.
**Used in chapters:** 1

### M03
**Misconception:** Fewer instructions always means faster code.
**Correct intuition:** Instruction count is one factor; stalls, vector width, memory behavior, and frequency also matter.
**Evidence that distinguishes:** Compare two functions' instruction counts *and* their measured elapsed time under the same conditions; a function with more instructions can still finish faster if its instructions are cheaper or better pipelined.
**Used in chapters:** 6 (introduced), 7 (revisited via the IPC/CPI framing that explains why)

### M04
**Misconception:** Higher IPC always means better performance.
**Correct intuition:** IPC describes pipeline utilization for a workload; elapsed time and completed work remain primary.
**Evidence that distinguishes:** Compare elapsed time and completed work directly, alongside IPC, rather than ranking implementations by IPC alone.
**Used in chapters:** 7

### M05
**Misconception:** A high cache-miss percentage proves a cache bottleneck.
**Correct intuition:** Rates need access volume, miss cost, overlap, and workload context.
**Used in chapters:** not yet (Chapter 17)

### M06
**Misconception:** A flame graph is a timeline.
**Correct intuition:** It aggregates stack samples; horizontal position is not chronological.
**Evidence that distinguishes:** Generate a flame graph from a workload with two independent threads (as in Chapter 14's lab); adjacent towers sit side by side by frame-name sort order, not by which one executed first or more recently.
**Used in chapters:** 14

### M07
**Misconception:** The widest frame is the function to optimize.
**Correct intuition:** Width may represent necessary work, a wrapper, or accumulated child cost.
**Evidence that distinguishes:** Check whether a wide frame is a leaf (real self cost) or has children spanning nearly its full width (a wrapper passing width through) before deciding it's the target — Chapter 14's lab's `thread_start`/`_pthread_start` frames are wide but do essentially no work of their own.
**Used in chapters:** 14

### M31 — proposed, pending review
**Misconception:** A flame graph's frame height or color encodes how expensive or "hot" a function is.
**Correct intuition:** Only width encodes magnitude (share of samples); height encodes stack depth (how many calls deep a frame sits), and color is only ever a visual distinguisher between adjacent frames, never a second magnitude scale.
**Evidence that distinguishes:** Two frames at the same depth with very different sample counts get different widths but the same height; two adjacent frames with nearly identical sample counts can be rendered in visually different colors purely because the renderer's palette differs by name, not by cost.
**Used in chapters:** 14

### M32 — proposed, pending review
**Misconception:** A narrow frame in a CPU flame graph can't matter to tail latency.
**Correct intuition:** A CPU flame graph only shows on-CPU sampled execution (M08); a rare, narrow frame can still be on the critical path of a slow request, and a genuinely tail-latency-dominating cause may be off-CPU entirely and invisible to a CPU flame graph no matter how the reader squints at frame widths.
**Evidence that distinguishes:** Chapter 11's opening story directly: a rare 500ms event a few times an hour would appear, at best, as a vanishingly narrow sliver (or nothing at all) in a CPU flame graph, while dominating a fraction of users' worst-case latency.
**Used in chapters:** 14

### M08
**Misconception:** Sampling profiles show all latency.
**Correct intuition:** They primarily show on-CPU execution unless an off-CPU method is used.
**Evidence that distinguishes:** A CPU profile's own off-CPU-adjacent frames (e.g. a thread blocked in `pthread_join`) show only that a thread was scheduled onto something, not why it was blocked or for how long relative to a request's actual latency; full off-CPU accounting needs the tools Chapter 29 introduces.
**Used in chapters:** 12 (introduced); full off-CPU treatment Chapter 29

### M09
**Misconception:** Pinning always improves performance.
**Correct intuition:** Affinity can reduce migrations or increase queueing and imbalance.
**Used in chapters:** not yet (Chapter 23)

### M10
**Misconception:** CPU affinity also binds memory.
**Correct intuition:** CPU and memory placement are separate policies.
**Used in chapters:** not yet (Chapter 23/24)

### M11
**Misconception:** NUMA matters only at enormous scale.
**Correct intuition:** Any multi-node system can suffer remote-memory cost and bandwidth imbalance.
**Used in chapters:** not yet (Chapter 24)

### M12
**Misconception:** Local memory is always optimal.
**Correct intuition:** Interleaving can improve aggregate bandwidth; placement depends on access pattern.
**Used in chapters:** not yet (Chapter 25)

### M13
**Misconception:** eBPF has zero overhead.
**Correct intuition:** Overhead depends on hook rate, work per event, stack capture, aggregation, and output.
**Used in chapters:** not yet (Chapter 27)

### M14
**Misconception:** More tracing produces more truth.
**Correct intuition:** Excess event volume can perturb the workload and bury the useful signal.
**Used in chapters:** not yet (Chapter 26/28)

### M15
**Misconception:** One benchmark run is evidence.
**Correct intuition:** One run is an anecdote unless the effect is overwhelming and the environment is controlled.
**Evidence that distinguishes:** Run the same configuration repeatedly and interleaved with its comparison; if repeat runs of the *same* configuration spread nearly as much as the two configurations differ, a single run proves nothing.
**Used in chapters:** 4

### M16
**Misconception:** A microbenchmark improvement guarantees a production improvement.
**Correct intuition:** Production may have a different workload, bottleneck, concurrency pattern, or critical path.
**Used in chapters:** not yet (Chapter 30)

### M17
**Misconception:** An optimization is complete when the original hotspot shrinks.
**Correct intuition:** The workload outcome must improve, and the bottleneck may move elsewhere.
**Used in chapters:** not yet (Chapter 15/30)

### M18
**Misconception:** Vendor peak bandwidth is the expected application bandwidth.
**Correct intuition:** Sustainable bandwidth depends on channels, access pattern, concurrency, instructions, and platform configuration.
**Used in chapters:** not yet (Chapter 19)

### M19
**Misconception:** Context-switch counts alone diagnose scheduler overhead.
**Correct intuition:** The impact depends on why switches occur, where the critical thread waits, and what locality is lost.
**Used in chapters:** not yet (Chapter 22)

### M20
**Misconception:** A profiler's output is ground truth.
**Correct intuition:** Every profiler is a measurement system with scope, overhead, permissions, and blind spots.
**Evidence that distinguishes:** Compare a tool's reported number against an independent measurement of the same quantity (e.g. wall time vs. a summed sampled estimate); disagreement reveals the tool's own scope and blind spots.
**Used in chapters:** touched on in Chapter 4 (trusting benchmark tooling) and Chapter 10 (multiplexed/scaled `perf stat` counters); full treatment Chapter 20

### M21 — proposed, pending review
**Misconception:** A program has one true performance number.
**Correct intuition:** "Faster" is only meaningful for a stated workload, input, and metric; the same change can help one input size or metric and hurt another.
**Evidence that distinguishes:** Run the same change across multiple input sizes or metrics (e.g. total time vs. p99 latency) and show the ranking of "which version is faster" flips.
**Used in chapters:** 2

### M22 — proposed, pending review
**Misconception:** 100% utilization is always bad, and any utilization below 100% means there's no problem.
**Correct intuition:** Utilization and saturation are different measurements; a resource can be moderately utilized and still heavily saturated (queueing) under bursty or correlated arrivals, and full utilization is sometimes the deliberate goal (e.g. a batch job).
**Evidence that distinguishes:** Drive a system at increasing concurrency and plot utilization alongside queue growth/latency; saturation can appear before utilization visibly reaches 100%, and utilization near 100% is not itself evidence of a problem for a throughput-oriented workload.
**Used in chapters:** 3

### M23 — proposed, pending review
**Misconception:** Changing several things at once and observing an improvement proves which change mattered.
**Correct intuition:** Without changing one thing at a time and testing a falsifiable hypothesis, an improvement after a multi-part change cannot be attributed to any specific part of it — it could even be masking a regression in one part.
**Evidence that distinguishes:** Revert changes one at a time (or apply them one at a time from baseline) and re-measure; if the ranking of "which single change explains the improvement" is undefined or contradictory, the original multi-change comparison was not diagnostic.
**Used in chapters:** 5

### M24 — proposed, pending review
**Misconception:** A pipeline stall means the CPU is broken or misconfigured.
**Correct intuition:** Stalls are a normal, expected consequence of a workload's dependency structure and memory access pattern, not a hardware fault — a CPU with zero stalls would require unlimited independent work, which essentially never occurs in practice.
**Evidence that distinguishes:** Run the exact same CPU on the exact same instruction mix with varying amounts of independent work available (e.g. `cyclelab compute --chains=N` at increasing N); stall behavior changes dramatically with zero hardware changes, showing the workload's shape, not the hardware, is the variable.
**Used in chapters:** 8

### M25 — proposed, pending review
**Misconception:** A branchless implementation is always faster.
**Correct intuition:** Removing a branch replaces an occasional misprediction cost with a guaranteed, unconditional cost — worthwhile only when the original branch was actually expensive (frequently mispredicted), and a net loss when it wasn't.
**Evidence that distinguishes:** Measure the original branch's actual predictability on realistic data (or elapsed time as a proxy, absent a misprediction counter) before assuming a branchless rewrite will win; a highly predictable branch is already nearly free, and a branchless rewrite there adds guaranteed cost to avoid a rarely-paid penalty.
**Used in chapters:** 9

### M26 — proposed, pending review
**Misconception:** More requested performance-counter events always means a more complete picture.
**Correct intuition:** The PMU has a limited number of physical counter registers; requesting more events than fit forces multiplexing, degrading the precision of every requested event rather than adding a free extra dimension.
**Evidence that distinguishes:** Compare a small, targeted event list's `perf stat` output against a large, unfocused one on the same command, and check the reported scaling percentages in the second case.
**Used in chapters:** 10

### M27 — proposed, pending review
**Misconception:** Adding more CPU cores or threads fixes a dependency-chain-limited workload.
**Correct intuition:** A single dependency chain running on one thread is bound by that thread's own available instruction-level parallelism, not by how many other cores exist on the machine; more cores help independent tasks that can run in parallel, not one sequential chain of dependent operations.
**Evidence that distinguishes:** Compare `cyclelab compute --chains=1` run with `--threads=1` against the same run with `--threads=8` on an 8-core machine; per-thread throughput for the single dependency chain does not improve, because the bottleneck was never a shortage of cores.
**Used in chapters:** 8

### M28 — proposed, pending review
**Misconception:** Sorting data specifically to help the branch predictor is always worth the sorting cost.
**Correct intuition:** Sorting has its own real cost (at best O(n log n) comparisons); for a single linear pass over the data, that cost can easily exceed whatever misprediction penalty it saves, and is only clearly worthwhile when the same sorted order is reused across many passes.
**Evidence that distinguishes:** Compare total time for "sort then scan once" against "scan once unsorted" on the same data; the sorted version can lose even though its scan phase alone is faster, once the sort's own cost is included.
**Used in chapters:** 9

### M29 — proposed, pending review
**Misconception:** The function with the highest cost in a profile is always the best optimization target.
**Correct intuition:** A high *inclusive* cost can come entirely from a function's position in the call graph (called from many otherwise-unrelated places) rather than from its own code being expensive; only self cost points at the function's own code.
**Evidence that distinguishes:** Check a profile's self cost, not just inclusive cost, before deciding where to spend optimization effort — a function with high inclusive cost but low self cost points at its callees, not at itself.
**Used in chapters:** 12

### M30 — proposed, pending review
**Misconception:** Kernel frames in a profile are irrelevant to application performance.
**Correct intuition:** Time an application's own code causes to be spent in the kernel (a syscall, a page fault, a scheduling decision) is still time that request or workload waited for, even though the executing code isn't the application's own.
**Evidence that distinguishes:** Compare a profile with and without kernel frames included; a kernel-side frame reached only because of an application's own call (e.g. blocking in a syscall the application invoked) still explains real elapsed time for that workload.
**Used in chapters:** 12
**Used in chapters:** 9
