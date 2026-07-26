# Concept Dependency Graph

Human-readable mirror of [`concept-graph.yaml`](concept-graph.yaml), which is
the machine-readable source of truth. Both derive from BLUEPRINT.md Section
11 (Concept Dependency Graph) and Section 12 (Narrative Graph).

**Dependency rule:** a concept at level *N* may assume all concepts at
levels *< N*. No chapter may rely on a concept before its prerequisites have
been introduced (BLUEPRINT.md Section 11, closing line).

Status as of this revision: **Chapters 1–30 drafted — the full book is complete.**

## Level 0 — Questions and Measurements

workload¹ · metric · latency³ · throughput³ · utilization³ · saturation³ ·
variance⁴ · distribution⁴ · baseline⁵ · experiment⁵ · confounder⁴ ·
measurement overhead

Superscript = chapter that formally introduces the term; no superscript =
not yet introduced.

## Level 1 — Linux Execution

process* · thread* · runnable²¹ · running²¹ · sleeping²¹ · blocked²¹ ·
scheduler²¹ · run queue²¹ · context switch²² · CPU migration²² ·
user time · system time · wall time

Ten of thirteen Level 1 terms are now introduced (Chapters 21-22). \*
**process** and **thread** are used informally from Chapter 1 onward but
never given a dedicated formal definition of their own — they're
foundational enough that the book treats them as already understood
rather than as a "new concept" to introduce. **user time**, **system
time**, and **wall time** remain informal: Chapter 1's guided lab
compares them operationally (for a `cyclelab compute` run) without a
formal scheduler model; Chapter 21 builds that model but reasons in
terms of running/runnable/sleeping rather than revisiting these three
terms directly.

## Level 2 — CPU Work

instruction⁶ · cycle⁷ · retired instruction⁶ · IPC⁷ · CPI⁷ · pipeline⁸ ·
front end⁸ · back end⁸ · branch prediction⁹ · speculation⁹ · dependency⁸ ·
stall⁸ · PMU¹⁰ · hardware performance event¹⁰ · multiplexing¹⁰

All fifteen Level 2 terms are now introduced (Chapters 6–10 complete
Part II). A few used a slightly different surface form than the Section
11 seed name: "instruction" as Chapter 6's "machine instruction",
"pipeline" as Chapter 8's title/Core Intuition (its two halves, front
end/back end, are Chapter 8's literal New Concepts entries), "branch
prediction"/"speculation" as Chapter 9's "branch predictor"/"speculative
execution", and "hardware performance event" as Chapter 10's "hardware
event". See each chapter's "New concepts:" header line for the exact
wording used.

## Level 3 — Profiling

counter¹¹\* · sampling¹¹ · tracing¹¹ · sample frequency¹² · call stack¹²\* ·
symbol¹³ · debug information¹³ · unwinding¹³ · frame pointer¹³ ·
call graph¹² · flame graph¹⁴ · differential flame graph¹⁵ · annotation¹²

All thirteen Level 3 terms are now introduced (Chapters 11–15 complete
Part III). \* **counter** was used operationally in Chapter 10 (PMU
counters, `perf stat`) before Chapter 11 formally contrasts counting
with sampling and tracing as three observation models — the "informal
early, formal later" pattern also used for the Level 7 terms below.
**call stack** is used from Chapter 11 onward and first substantively
built on via Chapter 12's "call graph" (a tree of call stacks). A few
others used a different surface form than the seed name: "symbol" as
Chapter 13's "symbol table", "unwinding" as "stack unwinding", "frame
pointer" as "frame pointers", "sample frequency" via Chapter 12's
separate "sample" and "frequency" entries.

## Level 4 — Memory Behavior

cache line¹⁶ · L1/L2/LLC¹⁶ · locality¹⁶ · working set¹⁷ · cache hit¹⁷ ·
cache miss¹⁷ · prefetching¹⁷ · coherence¹⁸ · false sharing¹⁸ ·
memory-level parallelism¹⁹ · memory bandwidth¹⁹ · memory latency¹⁶ ·
uncore²⁰ · roofline intuition¹⁹

All fourteen Level 4 terms are now introduced (Chapters 16–20 complete
Part IV). A few used a different surface form than the seed name:
"cache miss" as Chapter 17's "compulsory miss"/"capacity miss
intuition", "prefetching" as "prefetcher", "memory bandwidth" as
"sustained bandwidth"/"peak bandwidth", "memory latency" as Chapter
16's plain "latency".

## Level 5 — Topology and Placement

core²³ · logical CPU²³ · SMT²³ · socket²³ · CPU affinity²³ · cpuset²³ ·
cgroup²³ · isolation²³ · NUMA node²⁴ · NUMA distance²⁴ ·
first-touch allocation²⁵ · local memory²⁴ · remote memory²⁴ ·
memory policy²⁵ · page migration²⁵

All fifteen Level 5 terms are now introduced (Chapters 23-25 complete
the topology/placement material in Part V, alongside Chapters 21-22's
Level 1 scheduling material). A few used a different surface form:
"core" as "physical core", "SMT" as "SMT sibling", "cgroup" as "cgroup
CPU constraints", "NUMA distance" as "distance", "memory policy" via
"local allocation policy"/"interleave"/"bind"/"preferred node"
collectively.

## Level 6 — Dynamic Tracing

event²⁶ · tracepoint²⁶ · kprobe²⁶ · kretprobe²⁶ · uprobe²⁶ · USDT²⁶ ·
eBPF program²⁷ · verifier²⁷ · map²⁷ · helper²⁷ · attachment point²⁷ ·
ring buffer²⁷ · histogram²⁸ · BTF²⁷ · CO-RE²⁷ · BCC²⁸ · bpftrace²⁸

All seventeen Level 6 terms are now introduced (Chapters 26–28 complete
the tracing/eBPF material in Part VI). "attachment point" is introduced
via Chapter 27's surface term "hook" — BLUEPRINT.md's own Chapter 27
"Introduces:" line uses "hook", while the Level 6 seed list uses
"attachment point"; same concept.

## Level 7 — Whole-System Diagnosis

on-CPU time²⁹\* · off-CPU time²⁹\* · wake-up latency²⁹ · lock contention²⁹ ·
futex²⁹ · I/O latency²⁹ · queueing delay²⁹ · interference³⁰\* ·
bottleneck shift³⁰\* · regression³⁰ · scalability³⁰ · tail latency³ · causal claim³⁰\*

All thirteen Level 7 terms are now introduced. \* = used
informally/operationally well before its formal chapter (see "Resolved
tensions" below), then formally completed in the chapter marked. "futex"
is introduced via "futex wait"; "I/O latency" via "block I/O"
(BLUEPRINT.md's own Chapter 29 "Introduces:" line uses "block I/O", the
Level 7 seed list uses "I/O latency"; same concept).

## Resolved tensions between the Level 7 list and the early chapter outlines

BLUEPRINT.md's own per-chapter "Introduces:" lists put a handful of Level 7
terms to work well before Part VI formally builds them. All are now
formally completed, each in the chapter noted:

| Term | Formal chapter | Used informally in | Why this was not an error |
|---|---|---|---|
| on-CPU / off-CPU | 29 | Chapter 1, Chapter 21 | Chapter 1's whole purpose is to break the "100%-CPU-or-nothing" assumption; it needs the *words* on-CPU/off-CPU as everyday vocabulary before Chapter 29 builds the full off-CPU/wake-up/futex model. |
| tail latency | 3 (formal home; Level 7's whole-system sense extended, not re-introduced, in 29) | Chapter 3 (formal "Introduces" line) | Chapter 3's Section 11 outline explicitly lists `tail latency` as a *new* Ch3 concept, so its p50/p99 sense is treated as introduced there; Chapter 29's off-CPU model extends it to whole-system waiting, not just queueing, without a second formal introduction. |
| interference | 30 | Chapter 22 (title, "noisy neighbor" lab) | Chapter 22's whole guided lab is built around interference between co-located workloads; Chapter 30's Core Intuition formally names and defines the concept, tying back to Chapter 22 explicitly. |
| bottleneck shift, causal claim | 30 | Chapter 5 (investigation loop) | The 10-step loop (Section 5) uses both terms operationally as loop steps; Chapter 30's Core Intuition gives both a full formal definition, and the chapter's own Key Takeaway is itself a definition of a defensible causal claim. |
| counter | 3 (Ch11) | Chapter 10 (`perf stat`) | Chapter 10 is entirely about reading PMU counters via `perf stat`, well before Part III formally defines "counter" as a Level 3 profiling concept alongside sampling and tracing; Chapter 10 needs the word operationally and Chapter 11 is where counting is formally contrasted with sampling and tracing as three observation models. |

This mirrors BLUEPRINT.md's own teaching philosophy (Section 7.3: "a chapter
may use previously established concepts freely but should introduce only one
load-bearing idea") — these are cases where a plain-language sense of a term
is genuinely needed early, and its precise, mechanism-level sense is built
later. `validate_concept_graph.py` (currently a stub, see `scripts/`) should
treat entries with `also_appears_in` as intentional, not violations.

## Supplementary vocabulary (not in Section 11 at any level)

Chapters 1–30 also introduce plain teaching vocabulary that Section 11 never
lists at all. These are tracked in `glossary.md` but have no `level` in
`concept-graph.yaml`:

- **Chapter 1:** resource, bottleneck, critical path
- **Chapter 2:** operation, workload model, response time, completion time, service level, capacity, cost per unit of work
- **Chapter 3:** concurrency, queue
- **Chapter 4:** warm-up, repetition, outlier, randomization, thermal state, frequency scaling, background interference, confidence interval intuition
- **Chapter 5:** hypothesis, falsification, scope, perturbation
- **Chapter 6:** compiler, machine instruction, micro-operation intuition, optimization, vectorization, inlining, dead-code elimination
- **Chapter 7:** instruction count, elapsed cycles, reference cycles, CPU frequency
- **Chapter 8:** decode, execution units, out-of-order execution, memory wait, issue width intuition
- **Chapter 9:** branch, branch predictor, misprediction, speculative execution, dependency chain, branchless trade-off
- **Chapter 10:** hardware event, software event, event group, scaling, per-thread versus system-wide measurement, privilege restrictions
- **Chapter 11:** counting, sampling (as an observation model), tracing (as an observation model)
- **Chapter 12:** sample, period, overhead percentage, inclusive versus self cost, source mapping
- **Chapter 13:** stripped binary, DWARF, last branch records, JIT symbols, kernel symbols
- **Chapter 14:** folded stack, stack aggregation, frame width, ancestry, plateau, tower, off-CPU flame graph
- **Chapter 15:** before/after profile, normalized workload, `perf diff`, regression, total-work normalization
- **Chapter 16:** register, DRAM (as plain terms alongside their Level 4 counterparts)
- **Chapter 17:** stride, reuse, TLB (introduced as "a forward pointer")
- **Chapter 18:** shared cache line, ownership, invalidation, coherence traffic, true sharing, HITM intuition
- **Chapter 19:** bytes transferred, sustained bandwidth, peak bandwidth, arithmetic intensity, bandwidth saturation
- **Chapter 20:** load/store sampling, data source, local/remote classification, cache-to-cache transfer, memory controller
- **Chapter 21:** time slice intuition, wake-up, load balancing, scheduling class
- **Chapter 22:** cache warmth, interrupt, steal time, noisy neighbor, CPU pressure
- **Chapter 23:** affinity mask, `taskset`, `sched_setaffinity`
- **Chapter 24:** interconnect, node topology, memory-only node
- **Chapter 25:** local allocation policy, interleave, bind, preferred node, automatic NUMA balancing, NUMA hit/miss statistics
- **Chapter 26:** function entry/return, argument capture, event rate
- **Chapter 27:** per-CPU map, user-space loader
- **Chapter 28:** probe specification, predicate, action, aggregation, stack aggregation, interval output
- **Chapter 29:** blocked stack, sleep, block I/O, off-CPU flame graph (first informally named in Chapter 14, formally defined here)
- **Chapter 30:** (none — pure synthesis; see the Level 7 "Resolved tensions" table above for the five terms it formally completes)

## Narrative Graph (Section 12)

The book follows one chain of 30 questions, each becoming the next chapter's
opening question. See `concept-graph.yaml`'s `narrative_graph` list for the
full machine-readable form (question text + chapter number). All 30
questions are answered by the drafted chapters — the chain is complete.
The chain is verified end-to-end for all 30 chapters: each chapter's "Next
Obvious Question" is the verbatim opening question of the chapter that
follows, including the Chapter 5 → Chapter 6 handoff ("What work does the CPU
actually execute?"), the Chapter 10 → Chapter 11 handoff ("When should
we count, sample, or trace?"), the Chapter 15 → Chapter 16 handoff
("Why can memory access dominate code that performs little
computation?"), the Chapter 20 → Chapter 21 handoff ("How does Linux
decide where runnable work executes?"), and the Chapter 25 → Chapter 26
handoff ("What can dynamic tracing observe that counters and sampling
cannot?"). Six of BLUEPRINT.md's own per-chapter "Next question:"
shorthand lines (Chapter 8's, Chapter 10's, and — notably, every single
one in Part VI: Chapter 26's, 27's, 28's, and 29's) do not match the
following chapter's actual opening question verbatim; in every case the
drafted chapter uses the next chapter's real opening question, not the
shorthand paraphrase, per the rule established handling the Chapter 5 →
6 transition. Concretely: Chapter 26's shorthand ("How can custom
tracing logic execute safely in a running kernel?") differs from Chapter
27's real opening ("How can eBPF safely run custom measurements inside
the kernel?"); Chapter 27's shorthand ("How can an engineer use that
model without writing a full libbpf application?") differs from Chapter
28's real opening; Chapter 28's shorthand ("How do we explain time that
CPU profiles cannot see?") differs from Chapter 29's real opening
("Where does time go when a thread is not on a CPU?"); and Chapter 29's
shorthand ("How do we combine workload design, counters, profiles,
memory analysis, topology, and tracing into one investigation?") differs
from Chapter 30's real opening ("How do all the layers combine into one
defensible investigation?"). All four Part VI chapters use the real
opening question of the chapter that follows, not the shorthand.
