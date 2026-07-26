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
**Used in chapters:** 1 (introduced); revisited in Chapter 19 for memory-bandwidth-saturated CPUs specifically

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
**Evidence that distinguishes:** Connect a miss count (or an elapsed-time proxy for it) to total access volume and completed work before concluding anything is a bottleneck — a high rate over few accesses can matter less than a lower rate over many.
**Used in chapters:** 17

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
**Used in chapters:** 12 (introduced); revisited 26; full off-CPU treatment and resolution Chapter 29

### M09
**Misconception:** Pinning always improves performance.
**Correct intuition:** Affinity can reduce migrations or increase queueing and imbalance.
**Evidence that distinguishes:** Compare a workload's performance pinned versus unpinned at its actual thread count, not assumed from a different workload's earlier success with pinning — a pinned set too small for the thread count increases queueing instead of reducing it.
**Used in chapters:** 23

### M10
**Misconception:** CPU affinity also binds memory.
**Correct intuition:** CPU and memory placement are separate policies.
**Evidence that distinguishes:** Chapter 24/25's NUMA placement tools (`numactl --membind`, etc.) operate independently of CPU affinity tools (`taskset`), precisely because the two are orthogonal — pinning CPUs alone leaves memory placement exactly where it was.
**Used in chapters:** 23 (introduced); revisited 25

### M11
**Misconception:** NUMA matters only at enormous scale.
**Correct intuition:** Any multi-node system can suffer remote-memory cost and bandwidth imbalance.
**Evidence that distinguishes:** Check node count directly (`numactl --hardware`) rather than assuming from a machine's size or role — a compact dual-socket server has exactly the same local/remote distinction as a much larger one.
**Used in chapters:** 24

### M12
**Misconception:** Local memory is always optimal.
**Correct intuition:** Interleaving can improve aggregate bandwidth; placement depends on access pattern.
**Used in chapters:** not yet (Chapter 25)

### M13
**Misconception:** eBPF has zero overhead.
**Correct intuition:** Overhead depends on hook rate, work per event, stack capture, aggregation, and output.
**Evidence that distinguishes:** Chapter 27's own overhead equation: every helper call, map update, and emitted event has a real cost, multiplied by the underlying hook's event rate (Chapter 26) — a program on a rarely-firing hook costs little; the same program on a hot path can be expensive.
**Used in chapters:** 27

### M14
**Misconception:** More tracing produces more truth.
**Correct intuition:** Excess event volume can perturb the workload and bury the useful signal.
**Evidence that distinguishes:** compare a narrowly scoped probe (one function, one specific predicate) against an unfiltered, everything-that-moves trace of the same workload — the unfiltered version costs more overhead, produces far more data to sift through, and answers the original question no better (Chapter 26); a predicate that narrows a `bpftrace` one-liner to the specific condition under investigation is usually the difference between an inspectable question and an accidentally expensive one (Chapter 28).
**Used in chapters:** 26 (introduced); revisited 28

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
**Evidence that distinguishes:** The same metric (involuntary switches) means something different depending on whether it's elevated from genuine contention or not — compare a benchmark alone versus under real interference (Chapter 22's own lab) to see the same count carry different implications. Chapter 29's lab sharpens this further: on this book's reference machine, involuntary switches climbed sharply under real lock contention while voluntary switches stayed at a confirmed-flat zero in every workload tested — a captured mutex-wait stack, not the switch count, is what actually proves the blocking.
**Used in chapters:** 22 (introduced); revisited 29

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

### M33 — proposed, pending review
**Misconception:** A working set that fits in cache is automatically fast.
**Correct intuition:** Fitting in cache is necessary but not sufficient — an access pattern that defeats spatial or temporal locality can still perform poorly even within a cache level's capacity.
**Evidence that distinguishes:** Compare two access patterns over the exact same, cache-resident working set but different orders (e.g. Chapter 17's stride sweep); a poorly-ordered pattern can still cost far more per access than a well-ordered one at identical size.
**Used in chapters:** 16

### M34 — proposed, pending review
**Misconception:** Padding every shared structure is a safe, free optimization.
**Correct intuition:** Padding increases memory footprint, sometimes drastically, and applying it to data that isn't actually experiencing false sharing wastes memory and can hurt locality elsewhere for no benefit.
**Evidence that distinguishes:** Measure scaling with and without padding before applying it — Chapter 18's own lab shows padding helps when threads independently write to nearby counters, not as a universal rule to apply reflexively.
**Used in chapters:** 18

### M35 — proposed, pending review
**Misconception:** Flat, non-idle CPU utilization during a bandwidth-saturated workload means nothing is wrong.
**Correct intuition:** A core can be technically busy while making very little forward progress, stalled waiting for data the memory system can't deliver any faster — utilization measures busy time, not useful throughput (an extension of M02 into memory-bandwidth territory specifically).
**Evidence that distinguishes:** Chapter 19's own lab: adding threads past the bandwidth saturation point does not increase completed work, even though every added thread is technically running and utilization stays non-idle.
**Used in chapters:** 19

### M36 — proposed, pending review
**Misconception:** A single memory-related counter or tool can report "the memory bottleneck."
**Correct intuition:** Memory behavior spans latency, bandwidth, coherence, and topology, each requiring a different measurement; no single number aggregates all of them meaningfully.
**Evidence that distinguishes:** Chapter 20's own synthesis exercise: four genuinely different Part IV findings (latency, stride, coherence, bandwidth) from Chapters 16-19, none reducible to a shared single metric.
**Used in chapters:** 20

### M37 — proposed, pending review
**Misconception:** High CPU utilization means the machine is optimally scheduling its work.
**Correct intuition:** Utilization is an aggregate busy-time measure that says nothing about whether specific threads are waiting in the run queue behind others; a machine can be fully utilized while accumulating real, avoidable queueing delay for latency-sensitive work.
**Evidence that distinguishes:** Chapter 21's own lab: throughput (a rough utilization proxy) stays flat from 10 to 40 threads on a 10-CPU machine while involuntary context switches nearly triple, showing real additional contention utilization alone never surfaces.
**Used in chapters:** 21

### M38 — proposed, pending review
**Misconception:** eBPF is a background daemon.
**Correct intuition:** An eBPF program is loaded into the kernel and executes synchronously, in the context of whatever triggered its hook — there is no separate always-on service process doing the measuring; user-space tools like `bpftrace` and BCC are loaders and readers, not the thing collecting the data.
**Evidence that distinguishes:** Unload the user-space loader process (`bpftrace` exiting, for instance) and the attached program detaches too — there is no independent daemon left running the measurement in the background.
**Used in chapters:** 27

### M39 — proposed, pending review
**Misconception:** eBPF can safely execute arbitrary kernel code.
**Correct intuition:** The verifier specifically restricts eBPF programs to a provably bounded, provably memory-safe subset of what's possible; it cannot and does not run arbitrary kernel code, and a program that violates the verifier's constraints is rejected before it runs at all, not sandboxed at runtime.
**Evidence that distinguishes:** Chapter 27's own guided lab: an unbounded loop and an unchecked pointer read are both rejected at load time, before a single instruction of either ever executes.
**Used in chapters:** 27

### M40 — proposed, pending review
**Misconception:** CO-RE makes every program portable to every kernel.
**Correct intuition:** CO-RE adapts a compiled program to differences in struct *layout* using BTF information; it cannot invent a helper, map type, or hook that an older target kernel simply does not have at all.
**Evidence that distinguishes:** A CO-RE program built assuming a recent kernel feature still fails to load on a kernel that predates that feature — portability adjustments notwithstanding, missing features stay missing.
**Used in chapters:** 27

### M41 — proposed, pending review
**Misconception:** Maps are ordinary user-space hash maps.
**Correct intuition:** A map is a kernel-resident data structure, defined and sized at load time, accessed through a narrow, verified API from inside the eBPF program and a separate system-call interface from user space — not something a user-space program can resize or iterate however it likes.
**Evidence that distinguishes:** Per-CPU maps exist specifically to avoid a cost (cross-core coherence traffic, Chapter 18's false-sharing territory) an ordinary shared hash table would incur under concurrent update — a distinction that only makes sense for a kernel-resident structure, not an in-process one.
**Used in chapters:** 27

### M42 — proposed, pending review
**Misconception:** Off-CPU time is automatically waste.
**Correct intuition:** A thread waiting is frequently correct, intentional, or imposed by a dependency entirely outside the program's control — off-CPU time is information about where time went and why, not an automatic verdict.
**Evidence that distinguishes:** Compare an off-CPU duration against a plausible minimum for whatever it's waiting on — a network round trip has a physical floor; a lock held for exactly as long as its critical section's real work has a floor too — time far beyond that floor is where contention or a genuine problem, not necessity, is the likely explanation.
**Used in chapters:** 29

### M43 — proposed, pending review
**Misconception:** Aggregation (counting, grouping, histograms) requires eBPF or a kernel-level tracer.
**Correct intuition:** Counting events grouped by a key, and bucketing values into a histogram, are general data-reduction operations computable anywhere data exists; what eBPF specifically contributes is aggregating at the event itself, inside the kernel, avoiding a per-event user-space round trip — a real efficiency advantage, not a claim that the underlying arithmetic is otherwise impossible.
**Evidence that distinguishes:** Chapter 28's own guided lab computes the same count-grouped-by-key and histogram shapes entirely in user space, after the fact, from `cyclelab`'s own JSON output — real aggregation, no kernel tracer involved.
**Used in chapters:** 28
