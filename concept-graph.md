# Concept Dependency Graph

Human-readable mirror of [`concept-graph.yaml`](concept-graph.yaml), which is
the machine-readable source of truth. Both derive from BLUEPRINT.md Section
11 (Concept Dependency Graph) and Section 12 (Narrative Graph).

**Dependency rule:** a concept at level *N* may assume all concepts at
levels *< N*. No chapter may rely on a concept before its prerequisites have
been introduced (BLUEPRINT.md Section 11, closing line).

Status as of this revision: **Chapters 1–5 drafted, Chapters 6–30 pending.**

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

instruction · cycle · retired instruction · IPC · CPI · pipeline ·
front end · back end · branch prediction · speculation · dependency ·
stall · PMU · hardware performance event · multiplexing

Not yet introduced. First formal treatment: Part II (Chapters 6–10).

## Level 3 — Profiling

counter · sampling · tracing · sample frequency · call stack · symbol ·
debug information · unwinding · frame pointer · call graph · flame graph ·
differential flame graph · annotation

Not yet introduced. First formal treatment: Part III (Chapters 11–15).

## Level 4 — Memory Behavior

cache line · L1/L2/LLC · locality · working set · cache hit · cache miss ·
prefetching · coherence · false sharing · memory-level parallelism ·
memory bandwidth · memory latency · uncore · roofline intuition

Not yet introduced. First formal treatment: Part IV (Chapters 16–20).

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

This mirrors BLUEPRINT.md's own teaching philosophy (Section 7.3: "a chapter
may use previously established concepts freely but should introduce only one
load-bearing idea") — these are cases where a plain-language sense of a term
is genuinely needed early, and its precise, mechanism-level sense is built
later. `validate_concept_graph.py` (currently a stub, see `scripts/`) should
treat entries with `also_appears_in` as intentional, not violations.

## Supplementary vocabulary (not in Section 11 at any level)

Chapters 1–5 also introduce plain teaching vocabulary that Section 11 never
lists at all. These are tracked in `glossary.md` but have no `level` in
`concept-graph.yaml`:

- **Chapter 1:** resource, bottleneck, critical path
- **Chapter 2:** operation, workload model, response time, completion time, service level, capacity, cost per unit of work
- **Chapter 3:** concurrency, queue
- **Chapter 4:** warm-up, repetition, outlier, randomization, thermal state, frequency scaling, background interference, confidence interval intuition
- **Chapter 5:** hypothesis, falsification, scope, perturbation

## Narrative Graph (Section 12)

The book follows one chain of 30 questions, each becoming the next chapter's
opening question. See `concept-graph.yaml`'s `narrative_graph` list for the
full machine-readable form (question text + chapter number). Questions 1–5
are answered by the drafted chapters; 6–30 are pending. The chain is
verified end-to-end for Chapters 1–5: each chapter's "Next Obvious Question"
is the verbatim opening question of the chapter that follows, including the
Chapter 5 → Chapter 6 handoff ("What work does the CPU actually execute?").
