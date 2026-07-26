# Concept Dependency Graph

Human-readable mirror of [`concept-graph.yaml`](concept-graph.yaml), which is
the machine-readable source of truth. Both derive from BLUEPRINT.md Section
11 (Concept Dependency Graph) and Section 12 (Narrative Graph).

**Dependency rule:** a concept at level *N* may assume all concepts at
levels *< N*. No chapter may rely on a concept before its prerequisites have
been introduced (BLUEPRINT.md Section 11, closing line).

Status as of this revision: **Chapters 1–20 drafted, Chapters 21–30 pending.**

## Level 0 — Questions and Measurements

workload¹ · metric · latency³ · throughput³ · utilization³ · saturation³ ·
variance⁴ · distribution⁴ · baseline⁵ · experiment⁵ · confounder⁴ ·
measurement overhead

Superscript = chapter that formally introduces the term; no superscript =
not yet introduced.

## Level 1 — Linux Execution

process · thread · runnable · running · sleeping · blocked · scheduler ·
run queue · context switch · CPU migration · user time · system time ·
wall time

None formally introduced yet. Chapter 1's guided lab uses **user time**,
**system time**, and **wall time** operationally (comparing them for a
`cyclelab compute` run) without a formal scheduler model — that model
arrives in Part V (Chapters 21–23).

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

core · logical CPU · SMT · socket · CPU affinity · cpuset · cgroup ·
isolation · NUMA node · NUMA distance · first-touch allocation ·
local memory · remote memory · memory policy · page migration

Not yet introduced. First formal treatment: Part V (Chapters 21–25).

## Level 6 — Dynamic Tracing

event · tracepoint · kprobe · kretprobe · uprobe · USDT · eBPF program ·
verifier · map · helper · attachment point · ring buffer · histogram ·
BTF · CO-RE · BCC · bpftrace

Not yet introduced. First formal treatment: Part VI (Chapters 26–28).

## Level 7 — Whole-System Diagnosis

on-CPU time¹\* · off-CPU time¹\* · wake-up latency · lock contention ·
futex · I/O latency · queueing delay · interference · bottleneck shift⁵\* ·
regression · scalability · tail latency³ · causal claim⁵\*

\* = used informally/operationally before its formal chapter (see "Known
tensions" below); formal treatment for all Level 7 terms is Chapters 26–30.

## Known tensions between the Level 7 list and the early chapter outlines

BLUEPRINT.md's own per-chapter "Introduces:" lines put a handful of Level 7
terms to work well before Part VI–VII formally builds them:

| Term | Formal level | Used informally in | Why this is not an error |
|---|---|---|---|
| on-CPU / off-CPU | 7 (Ch29) | Chapter 1 | Chapter 1's whole purpose is to break the "100%-CPU-or-nothing" assumption; it needs the *words* on-CPU/off-CPU as everyday vocabulary before Chapter 29 builds the full off-CPU/wake-up/futex model. |
| tail latency | 7 | Chapter 3 (formal "Introduces" line) | Chapter 3's Section 11 outline explicitly lists `tail latency` as a *new* Ch3 concept, so its p50/p99 sense is treated as introduced there; Chapter 29's Level-7 sense extends it to whole-system waiting, not just queueing. |
| bottleneck shift, causal claim | 7 | Chapter 5 (investigation loop) | The 10-step loop (Section 5) uses both terms operationally as loop steps; Chapter 30 is where they get full technical treatment as diagnosis concepts. |
| counter | 3 | Chapter 10 (`perf stat`) | Chapter 10 is entirely about reading PMU counters via `perf stat`, well before Part III formally defines "counter" as a Level 3 profiling concept alongside sampling and tracing; Chapter 10 needs the word operationally and Chapter 11 is where counting is formally contrasted with sampling and tracing as three observation models. |

This mirrors BLUEPRINT.md's own teaching philosophy (Section 7.3: "a chapter
may use previously established concepts freely but should introduce only one
load-bearing idea") — these are cases where a plain-language sense of a term
is genuinely needed early, and its precise, mechanism-level sense is built
later. `validate_concept_graph.py` (currently a stub, see `scripts/`) should
treat entries with `also_appears_in` as intentional, not violations.

## Supplementary vocabulary (not in Section 11 at any level)

Chapters 1–20 also introduce plain teaching vocabulary that Section 11 never
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

## Narrative Graph (Section 12)

The book follows one chain of 30 questions, each becoming the next chapter's
opening question. See `concept-graph.yaml`'s `narrative_graph` list for the
full machine-readable form (question text + chapter number). Questions 1–20
are answered by the drafted chapters; 21–30 are pending. The chain is
verified end-to-end for Chapters 1–20: each chapter's "Next Obvious
Question" is the verbatim opening question of the chapter that follows,
including the Chapter 5 → Chapter 6 handoff ("What work does the CPU
actually execute?"), the Chapter 10 → Chapter 11 handoff ("When should
we count, sample, or trace?"), the Chapter 15 → Chapter 16 handoff
("Why can memory access dominate code that performs little
computation?"), and the Chapter 20 → Chapter 21 handoff ("How does Linux
decide where runnable work executes?"). Two of BLUEPRINT.md's own
per-chapter "Next question:" shorthand lines (Chapter 8's and Chapter
10's) do not match the following chapter's actual opening question
verbatim; in both cases the drafted chapter uses the next chapter's real
opening question, not the shorthand paraphrase, per the rule established
handling the Chapter 5 → 6 transition.
