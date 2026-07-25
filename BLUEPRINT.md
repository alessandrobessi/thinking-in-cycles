# Thinking in Cycles

## A Mental Model for Linux Performance

**Document:** `BLUEPRINT.md`  
**Status:** Founding design document  
**Working title:** *Thinking in Cycles*  
**Working subtitle:** *A Mental Model for Linux Performance*  
**Author:** Alessandro Bessi

---

## 1. Mission

Linux performance engineering is often taught as a bag of commands:

- run `top`;
- try `perf`;
- generate a flame graph;
- inspect cache misses;
- pin a process;
- use eBPF when everything else fails.

That approach produces operators who know tools but do not know what question each tool answers, what its measurements actually mean, or when its output is misleading.

This book takes the opposite approach. It builds one cumulative mental model of where time goes in a Linux system:

1. a workload creates work;
2. Linux schedules that work onto CPUs;
3. CPUs execute instructions through pipelines;
4. instructions depend on branches, caches, and memory;
5. threads compete for cores, cache lines, locks, bandwidth, and NUMA-local memory;
6. measurement tools observe different layers of that system, each with blind spots and overhead;
7. a performance claim is trustworthy only when the workload and experiment are controlled.

The reader should finish the book able to move from a vague complaint—“this is slow”—to a testable hypothesis, select the right measurement, interpret the evidence, make one change, and prove whether it helped.

The book is not a command reference. It is a durable way to think about Linux performance even as processors, kernels, and observability tools change.

---

## 2. Core Thesis

> Performance engineering is the disciplined process of explaining where time and resources go, then proving that a change improves the workload that actually matters.

The book is organized around four principles:

1. **Measure a defined workload, not an abstract program.**
2. **Form a hypothesis before reaching for a specialized tool.**
3. **Interpret measurements through a model of the hardware and operating system.**
4. **Treat every optimization as an experiment that can fail, regress, or move the bottleneck.**

---

## 3. Title Decision

### Recommended title

# Thinking in Cycles

The title matches the naming pattern of *Thinking in Tokens* and *Thinking in Packets*. A cycle is the basic unit behind much CPU analysis, but the book must repeatedly clarify that not all waiting consumes CPU cycles and not all performance problems are CPU-bound.

### Subtitle

**A Mental Model for Linux Performance**

### Strong alternatives

- *Thinking in Bottlenecks: A Mental Model for Linux Performance*
- *Where the Time Went: Understanding Linux Performance*
- *The Shape of Fast: A Mental Model for Systems Performance*
- *Thinking in Latency: How Linux Workloads Really Run*

Use *Thinking in Cycles* unless the manuscript becomes so focused on off-CPU latency, I/O, and distributed behavior that the title feels misleading.

---

## 4. Audience

The primary reader is an intelligent, technically capable engineer who uses Linux but has never built a complete performance model.

Typical readers include:

- backend and systems engineers;
- DevOps, SRE, and platform engineers;
- C, C++, Rust, Go, Java, or Python developers who need to understand native system behavior;
- infrastructure consultants;
- database, storage, networking, and high-performance-computing practitioners at the beginning of serious performance work;
- engineers who have used `top`, `strace`, or `perf` without feeling confident about the conclusions.

### Assumed knowledge

The reader should already be able to:

- use a Linux shell;
- understand processes, files, and basic permissions;
- compile or run a small program;
- read simple command output;
- recognize threads, functions, and system calls at a basic level.

### Not assumed

The book does not assume prior knowledge of:

- CPU microarchitecture;
- hardware performance counters;
- assembly language;
- kernel internals;
- eBPF;
- NUMA;
- statistics beyond ratios, distributions, and basic uncertainty.

The reader will see shell commands, short C examples, and occasional assembly fragments, but every example must be explained from first principles.

---

## 5. Reader Outcomes

By the end, the reader should be able to:

1. turn “the system is slow” into a precise latency, throughput, utilization, or scalability question;
2. distinguish on-CPU time, runnable time, blocked time, and external waiting;
3. create a benchmark that is repeatable enough to support a decision;
4. use `perf stat` to form CPU-level hypotheses;
5. use `perf record`, `perf report`, and `perf annotate` to locate hot code paths;
6. generate and correctly interpret CPU and off-CPU flame graphs;
7. reason about instructions, cycles, IPC, CPI, stalls, and branch behavior without treating any single counter as a verdict;
8. explain cache locality, working sets, coherence, false sharing, and memory bandwidth;
9. use `perf mem`, `perf c2c`, and vendor or uncore tools when the hardware supports them;
10. understand CPU topology, SMT, migrations, affinity, cpusets, and interference;
11. explain NUMA, first-touch allocation, local versus remote memory, and placement policies;
12. use tracepoints, kprobes, uprobes, BCC tools, and `bpftrace` to investigate dynamic behavior;
13. diagnose off-CPU latency, lock contention, scheduling delays, and I/O waits;
14. estimate measurement overhead and recognize observability blind spots;
15. prove an improvement with controlled before-and-after evidence;
16. communicate a performance finding without overstating what the data proves.

---

## 6. Non-Goals

This book is not:

- a complete reference for every `perf` subcommand;
- a Linux kernel development manual;
- an assembly optimization handbook;
- a compiler construction book;
- a hardware-specific tuning guide for one CPU generation;
- an eBPF programming reference;
- a production observability platform guide;
- a Kubernetes performance book;
- a database, network, storage, JVM, Go, or Python tuning book;
- a real-time Linux or low-latency trading manual;
- a list of “magic” kernel parameters;
- a promise that every workload can be improved by low-level tuning.

Runtime-specific, cloud-specific, and architecture-specific material belongs in appendices or clearly marked sidebars.

---

## 7. The Teaching Philosophy

### 7.1 Intuition before instrumentation

Every chapter introduces the phenomenon before the tool.

The reader learns what a CPU migration is before running `perf sched`. The reader learns why cache lines move before seeing `perf c2c`. The reader learns what off-CPU time means before running an eBPF off-CPU profiler.

A command should feel like an answer to a question the reader already has.

### 7.2 Evidence before advice

The book must never present tuning changes as folklore:

- “pin the process”;
- “disable SMT”;
- “use huge pages”;
- “change the CPU governor”;
- “bind memory locally”;
- “increase this sysctl.”

Every change must be framed as a hypothesis with possible benefits, costs, and failure modes.

### 7.3 One new mental model at a time

A chapter may use previously established concepts freely but should introduce only one load-bearing idea.

For example:

- the cache chapter can assume cycles and sampling;
- the false-sharing chapter can assume cache lines and coherence;
- the NUMA placement chapter can assume affinity and memory locality.

It must not introduce the scheduler, caches, counters, and NUMA in one chapter merely because one incident involved all four.

### 7.4 Prediction before measurement

Whenever possible, ask the reader to predict the result before running the command.

Examples:

- Which loop should have better locality?
- Which thread arrangement should create cache-line bouncing?
- Will pinning improve the mean, the variance, both, or neither?
- Should a remote-memory run change latency, bandwidth, or CPU utilization?
- What shape should the flame graph have if the hypothesis is correct?

This turns tool usage into model testing rather than screenshot collection.

### 7.5 Measurements have provenance

Every result shown in the book must identify enough context to be interpreted:

- CPU model and topology;
- kernel version;
- compiler and flags;
- workload input;
- process/thread count;
- CPU and memory placement;
- frequency/governor state when relevant;
- warm-up and repetition policy;
- measurement command;
- known virtualization or permission constraints.

Numbers without provenance are illustrations, not evidence.

### 7.6 Visuals are allowed, but must carry technical meaning

Unlike *Thinking in Tokens*, this book should use diagrams because several central ideas are spatial or structural:

- CPU topology;
- cache hierarchy;
- cache-line ownership;
- NUMA nodes and distances;
- flame graphs;
- latency distributions;
- roofline-style bandwidth intuition.

Rules for visuals:

- no decorative diagrams;
- no diagram may replace the prose explanation;
- every diagram must answer one explicit question;
- measurement-generated visuals must distinguish real output from schematic output;
- flame graphs must be taught as data visualizations, not as illustrations;
- diagrams must remain legible in grayscale and in print.

### 7.7 Code is a microscope, not the subject

Short programs exist to make hardware behavior visible. They should not become exercises in application architecture.

The reference lab code should prefer:

- plain C for controlled memory and threading experiments;
- shell for orchestration;
- optional Python for result processing and plots;
- small, readable functions;
- deterministic inputs where possible;
- explicit compiler flags;
- debug symbols retained for profiling.

Every important code example must state what behavior it is designed to reveal.

---

## 8. The Book’s Recurring Investigation

The manuscript should reuse one small experimental program across the book: **`cyclelab`**.

`cyclelab` is a command-line workload generator with modes that expose different performance phenomena without requiring a large application.

Suggested modes:

```text
cyclelab compute
cyclelab branch
cyclelab sequential-memory
cyclelab random-memory
cyclelab bandwidth
cyclelab false-sharing
cyclelab lock-contention
cyclelab syscall
cyclelab sleep
cyclelab numa
cyclelab mixed
```

### Design requirements

- written in readable C;
- built with debug symbols and optimized code;
- configurable duration, threads, data size, stride, and CPU placement;
- prints work completed and a checksum to prevent dead-code elimination;
- supports stable workloads long enough to profile;
- never claims to model a production application;
- includes deliberately slow variants and corrected variants;
- records its configuration in machine-readable form;
- runs on x86-64 and Arm64 where practical;
- gracefully reports unavailable features.

### Why a recurring program matters

The reader should not spend each chapter learning a new codebase. Reusing one lab makes changes in tool output easier to attribute to the phenomenon being studied.

A second recurring example should be a small request-serving application, such as an HTTP key-value service, used only when queueing, tail latency, concurrency, or off-CPU behavior needs a more realistic workload.

---

## 9. Standard Chapter Template

Every chapter should substantively contain the following sections. The visible headings may be loosened during final editing, but the conceptual elements are mandatory.

```markdown
# [Chapter Title]

**Part:** [Part Name]  
**Concept level:** [Level from concept graph]  
**Prerequisites:** [Prior concepts]  
**New concepts:** [Terms introduced here]

## Opening Question

One plain-language question, with no unexplained jargon.

## Incident or Real-World Story

A concrete situation in which the missing concept causes a wrong conclusion.

## Predict Before Measuring

A small question the reader can answer using the current mental model.

## Worked Example

A second grounding from a different angle. This may use `cyclelab`, a real command, or a compact dataset.

## Core Intuition

The simplest correct mental model.

## Technical Explanation

The precise mechanism, including limited formulas, code, diagrams, or hardware detail where necessary.

## Tool View

The smallest useful command sequence. Explain:
- what is measured;
- what is not measured;
- required permissions;
- likely overhead;
- portability limitations;
- common failure modes.

## Guided Lab

A reproducible exercise with:
- setup;
- command;
- expected qualitative result;
- interpretation;
- cleanup.

Never require the reader to reproduce an exact numeric value.

## Common Misconceptions

For each:
- why it is wrong;
- the correct intuition;
- what evidence would distinguish the alternatives.

## Practical Implications

How the concept changes a real investigation.

## Key Takeaway

One bold, memorable sentence.

## What to Remember

Five to eight bullets.

## Further Reading

Primary sources and authoritative references.

## The Next Obvious Question

The question that becomes the next chapter’s opening question.
```

---

## 10. Mathematical Policy

The book uses only the mathematics that improves reasoning.

Core formulas include:

```text
latency = elapsed time per operation
throughput = completed work / elapsed time
utilization = busy time / available time
speedup = old time / new time
IPC = instructions / cycles
CPI = cycles / instructions
cache miss rate = misses / relevant accesses
bandwidth = bytes transferred / elapsed time
arithmetic intensity = useful operations / bytes transferred
parallel efficiency = speedup / number of workers
```

Amdahl’s law may be introduced through a worked example, not as an equation-first section.

Rules:

- define every numerator and denominator;
- state the measurement scope;
- distinguish rates from counts;
- distinguish wall time from CPU time;
- never compare ratios collected from incompatible event groups;
- never imply precision beyond the experiment.

---

## 11. Concept Dependency Graph

### Level 0 — Questions and Measurements

- workload;
- metric;
- latency;
- throughput;
- utilization;
- saturation;
- variance;
- distribution;
- baseline;
- experiment;
- confounder;
- measurement overhead.

### Level 1 — Linux Execution

- process;
- thread;
- runnable;
- running;
- sleeping;
- blocked;
- scheduler;
- run queue;
- context switch;
- CPU migration;
- user time;
- system time;
- wall time.

### Level 2 — CPU Work

- instruction;
- cycle;
- retired instruction;
- IPC;
- CPI;
- pipeline;
- front end;
- back end;
- branch prediction;
- speculation;
- dependency;
- stall;
- PMU;
- hardware performance event;
- multiplexing.

### Level 3 — Profiling

- counter;
- sampling;
- tracing;
- sample frequency;
- call stack;
- symbol;
- debug information;
- unwinding;
- frame pointer;
- call graph;
- flame graph;
- differential flame graph;
- annotation.

### Level 4 — Memory Behavior

- cache line;
- L1/L2/LLC;
- locality;
- working set;
- cache hit;
- cache miss;
- prefetching;
- coherence;
- false sharing;
- memory-level parallelism;
- memory bandwidth;
- memory latency;
- uncore;
- roofline intuition.

### Level 5 — Topology and Placement

- core;
- logical CPU;
- SMT;
- socket;
- CPU affinity;
- cpuset;
- cgroup;
- isolation;
- NUMA node;
- NUMA distance;
- first-touch allocation;
- local memory;
- remote memory;
- memory policy;
- page migration.

### Level 6 — Dynamic Tracing

- event;
- tracepoint;
- kprobe;
- kretprobe;
- uprobe;
- USDT;
- eBPF program;
- verifier;
- map;
- helper;
- attachment point;
- ring buffer;
- histogram;
- BTF;
- CO-RE;
- BCC;
- `bpftrace`.

### Level 7 — Whole-System Diagnosis

- on-CPU time;
- off-CPU time;
- wake-up latency;
- lock contention;
- futex;
- I/O latency;
- queueing delay;
- interference;
- bottleneck shift;
- regression;
- scalability;
- tail latency;
- causal claim.

No chapter may rely on a concept before its prerequisites have been introduced.

---

## 12. Narrative Graph

The book follows one chain of questions:

1. Why can a program feel slow when no obvious resource is fully used?
2. What exactly does “faster” mean for this workload?
3. How do latency, throughput, utilization, and saturation differ?
4. How do we know a measured difference is real?
5. What investigation process prevents random tuning?
6. What work does the CPU actually execute?
7. What do cycles and instructions tell us?
8. Why can a CPU spend cycles without retiring useful work?
9. How do branches and dependencies disrupt execution?
10. How can counters turn a vague slowdown into a hypothesis?
11. When should we count, sample, or trace?
12. Which functions and code paths consume CPU time?
13. Why are call stacks sometimes missing or wrong?
14. How do flame graphs show the shape of work?
15. How do we prove that an optimization changed the right thing?
16. Why can memory access dominate code that performs little computation?
17. Why do access order and working-set size change performance?
18. How can independent threads slow each other through cache coherence?
19. How do we tell whether a workload is limited by memory bandwidth?
20. Which tools reveal cache and memory behavior?
21. How does Linux decide where runnable work executes?
22. How do migrations and neighboring workloads add noise and delay?
23. When does CPU affinity help, hurt, or merely hide a problem?
24. Why is some memory farther away than other memory?
25. How do allocation and execution placement interact on NUMA systems?
26. What can dynamic tracing observe that counters and sampling cannot?
27. How can eBPF safely run custom measurements inside the kernel?
28. How do `bpftrace` and BCC turn questions into live instrumentation?
29. Where does time go when a thread is not on a CPU?
30. How do all the layers combine into one defensible investigation?

---

# Part I — Measuring Reality

## Chapter 1 — Why Fast-Looking Code Runs Slowly

**Opening question:** Why can a program feel slow when no obvious resource is fully used?

**Purpose:** Break the assumption that a bottleneck must appear as one resource pinned at 100%.

**Story:** A service with low average CPU usage still has severe request latency because its workers alternate between lock contention, scheduling delay, and storage waits.

**Introduces:** workload, resource, on-CPU, off-CPU, bottleneck, critical path.

**Guided lab:** Run `cyclelab mixed`, compare wall time, user time, and system time, and observe that elapsed time is not explained by CPU time alone.

**Key takeaway:** **A slow workload is an accounting problem: the first task is to explain where its elapsed time went.**

**Next question:** Before looking for the bottleneck, what does “faster” mean?

---

## Chapter 2 — Performance Is a Question, Not a Number

**Opening question:** What exactly does “faster” mean for this workload?

**Purpose:** Establish that performance is defined by a workload and an outcome, not by a universal score.

**Introduces:** operation, workload model, response time, completion time, service level, capacity, cost per unit of work.

**Worked examples:**

- a batch job optimized for total completion time;
- an API optimized for p99 latency under a fixed request rate;
- a data pipeline optimized for records per second within a CPU budget.

**Guided lab:** Run the same program with three input sizes and show that a change can improve small inputs while hurting large ones.

**Key takeaway:** **A performance result is meaningful only when the workload, metric, and operating conditions are explicit.**

**Next question:** Which measurements describe different kinds of speed?

---

## Chapter 3 — Latency, Throughput, Utilization, and Saturation

**Opening question:** How do latency, throughput, utilization, and saturation differ?

**Purpose:** Build the vocabulary needed for every later investigation.

**Introduces:** latency, throughput, utilization, saturation, concurrency, queue, tail latency.

**Core example:** A supermarket checkout can serve many customers per hour while one customer still waits too long; a cashier can be busy without the system being at maximum sustainable throughput.

**Technical scope:** Little’s-law intuition may be included without formal queueing theory.

**Guided lab:** Drive a small HTTP service at increasing concurrency. Record throughput and p50/p99 latency. Identify the knee where added concurrency mostly creates waiting.

**Key takeaway:** **More work in flight can raise throughput until the system saturates, after which it mostly raises latency.**

**Next question:** How can we distinguish a real improvement from measurement noise?

---

## Chapter 4 — Noise, Variance, and Honest Benchmarks

**Opening question:** How do we know a measured difference is real?

**Purpose:** Introduce benchmarking hygiene before readers are tempted to trust tool output.

**Introduces:** warm-up, repetition, distribution, outlier, randomization, confounder, thermal state, frequency scaling, background interference, confidence interval intuition.

**Mandatory topics:**

- never trust one run;
- record the environment;
- separate setup from timed work;
- use realistic input;
- prevent dead-code elimination;
- randomize or interleave A/B runs;
- inspect distributions, not only averages;
- watch thermal throttling and CPU frequency;
- distinguish cold-cache and warm-cache questions;
- avoid shared CI runners for performance conclusions.

**Guided lab:** Compare two nearly identical loops using repeated, interleaved runs. Show that the result changes when run order is fixed and the machine warms up.

**Key takeaway:** **A benchmark is an experiment, and uncontrolled experiments produce confident stories about noise.**

**Next question:** What repeatable process should guide a performance investigation?

---

## Chapter 5 — The Performance Investigation Loop

**Opening question:** What investigation process prevents random tuning?

**Purpose:** Give the reader a method used throughout the rest of the book.

### The loop

1. define the workload and success metric;
2. reproduce the problem;
3. establish a baseline;
4. classify where time is going;
5. form one hypothesis;
6. choose the least invasive measurement that can test it;
7. change one thing;
8. rerun the controlled experiment;
9. check for bottleneck movement and regressions;
10. document the evidence and uncertainty.

**Introduces:** baseline, hypothesis, falsification, scope, perturbation, causal claim, bottleneck shift.

**Guided lab:** A deliberately slow `cyclelab` configuration is investigated with only wall time, CPU time, process state, and basic system counters. The reader must decide which specialized tool is justified next.

**Key takeaway:** **Performance work advances by eliminating explanations, not by accumulating tuning ideas.**

**Next question:** To reason about CPU-bound work, what is the processor actually doing?

---

# Part II — What the CPU Is Doing

## Chapter 6 — From Source Code to Retired Instructions

**Opening question:** What work does the CPU actually execute?

**Purpose:** Connect source code to machine instructions without turning the book into an assembly course.

**Introduces:** compiler, machine instruction, micro-operation intuition, retired instruction, optimization, vectorization, inlining, dead-code elimination.

**Worked example:** Compare an unoptimized and optimized build of the same loop. Show that source-level similarity does not imply identical machine work.

**Guided lab:** Build `cyclelab compute` with `-O0`, `-O2`, debug symbols, and optional vectorization reports. Inspect a small function with `objdump` or `perf annotate`.

**Misconception:** Fewer source lines mean less CPU work.

**Key takeaway:** **The CPU executes the compiler’s instruction stream, not the programmer’s visual impression of the source.**

**Next question:** How do we quantify that instruction stream?

---

## Chapter 7 — Cycles, Instructions, IPC, and CPI

**Opening question:** What do cycles and instructions tell us?

**Purpose:** Introduce the central ratios used in CPU analysis while emphasizing their limits.

**Introduces:** cycle, instruction count, IPC, CPI, elapsed cycles, reference cycles, CPU frequency.

**Worked example:** Two implementations:
- one retires fewer instructions but has poor memory behavior;
- one retires more instructions but finishes sooner.

**Guided lab:** Use `perf stat` to collect task-clock, cycles, instructions, and elapsed time for compute and memory modes.

**Mandatory cautions:**

- IPC is workload- and microarchitecture-dependent;
- a high IPC is not automatically good;
- a low IPC is not automatically bad;
- comparisons require compatible scopes;
- frequency changes complicate cycle interpretation.

**Key takeaway:** **Cycles and instructions describe how work executed, but neither is a performance verdict without elapsed time and workload context.**

**Next question:** Why can many cycles pass without useful instructions retiring?

---

## Chapter 8 — The Pipeline: Front End, Back End, and Stalls

**Opening question:** Why can a CPU spend cycles without retiring useful work?

**Purpose:** Build a durable, architecture-neutral pipeline model.

**Introduces:** front end, decode, execution units, back end, out-of-order execution, stall, dependency, memory wait, issue width intuition.

**Canonical analogy:** An assembly line whose later stations can work out of order when materials are available, but which still stops if instructions do not arrive or operands are missing.

**Guided lab:** Compare a dependency chain with multiple independent accumulators. Use counters only if the CPU supports meaningful events; the qualitative runtime difference is primary.

**Tool policy:** Top-down microarchitecture analysis may be introduced as a framework, but event names must be isolated in architecture-specific notes.

**Key takeaway:** **A modern CPU is fast when it can keep many independent operations moving; stalls are failures to supply that parallel work.**

**Next question:** What prevents the pipeline from seeing the right work soon enough?

---

## Chapter 9 — Branch Prediction, Speculation, and Dependencies

**Opening question:** How do branches and dependencies disrupt execution?

**Purpose:** Explain why control flow and data dependencies matter even when instruction counts look similar.

**Introduces:** branch, branch predictor, misprediction, speculative execution, dependency chain, branchless trade-off.

**Worked examples:**

- sorted versus random data through the same conditional;
- one accumulator versus several independent accumulators;
- a branchless rewrite that performs extra work and is not always faster.

**Guided lab:** Run `cyclelab branch` with predictable and unpredictable inputs. Measure elapsed time, instructions, branches, and branch misses where supported.

**Misconception:** A branchless implementation is always faster.

**Key takeaway:** **The pipeline performs best when it can predict control flow and find independent work; removing a branch helps only if the replacement costs less than the uncertainty.**

**Next question:** How can performance counters convert these mechanisms into testable hypotheses?

---

## Chapter 10 — `perf stat`: Turning Counters into a Hypothesis

**Opening question:** How can counters turn a vague slowdown into a hypothesis?

**Purpose:** Teach `perf stat` as a structured first CPU tool, not as a wall of numbers.

**Introduces:** PMU, hardware event, software event, event group, multiplexing, scaling, per-thread versus system-wide measurement, privilege restrictions.

**Core command progression:**

```bash
perf stat -- ./cyclelab compute
perf stat -r 10 -- ./cyclelab compute
perf stat -e cycles,instructions,branches,branch-misses -- ./cyclelab branch
perf stat -a -- sleep 10
```

Commands must be presented as examples whose available events vary by architecture and kernel.

**Interpretation workflow:**

1. verify elapsed and CPU time;
2. inspect context switches and migrations;
3. inspect instructions and cycles;
4. add only events relevant to the current hypothesis;
5. notice unsupported or multiplexed counters;
6. avoid reading a generic “cache-misses” event as a complete cache diagnosis.

**Permissions:** Explain `perf_event_paranoid`, `kptr_restrict`, and `CAP_PERFMON` without defaulting to “run everything as root.”

**Key takeaway:** **`perf stat` is most useful when a small, hypothesis-driven event set explains why two controlled runs differ.**

**Next question:** Counters summarize events, but how do we find the code paths responsible?

---

# Part III — Where the CPU Time Goes

## Chapter 11 — Counting, Sampling, and Tracing

**Opening question:** When should we count, sample, or trace?

**Purpose:** Separate three observation models before introducing profilers and eBPF.

**Introduces:**

- counting: how many;
- sampling: where execution tends to be;
- tracing: what happened in sequence.

**Worked example:** A service spends 40% of CPU in one path, but a rare 500 ms event causes tail latency. Sampling finds the hot path; tracing finds the rare sequence.

**Guided lab:** Compare `perf stat`, `perf record`, and one tracepoint-based command on the same workload.

**Mandatory topic:** Measurement overhead grows with event rate, stack capture, payload size, and output volume.

**Key takeaway:** **Choose the observation model that matches the question: counts summarize, samples locate, and traces reconstruct.**

**Next question:** How does sampling locate CPU-consuming functions?

---

## Chapter 12 — `perf record`, `perf report`, and `perf annotate`

**Opening question:** Which functions and code paths consume CPU time?

**Purpose:** Teach a minimal but complete CPU profiling workflow.

**Core workflow:**

```bash
perf record -g -- ./cyclelab compute
perf report
perf annotate
```

Later variants may include PID-scoped, CPU-wide, cgroup-scoped, frequency-controlled, and event-specific recording.

**Introduces:** sample, period, frequency, overhead percentage, inclusive versus self cost, call graph, annotation, source mapping.

**Guided lab:** Profile a workload with one deliberately expensive leaf function and one expensive calling path. Ask the reader to distinguish self time from accumulated time.

**Misconceptions:**

- the top function is necessarily the best optimization target;
- a sample percentage is exact elapsed time;
- a hot function is necessarily inefficient;
- kernel frames are irrelevant to application performance.

**Key takeaway:** **A CPU profile shows where sampled on-CPU execution accumulated, not why the code was slow or what happened while it was off CPU.**

**Next question:** Why do profiles sometimes have incomplete or misleading call stacks?

---

## Chapter 13 — Stacks, Symbols, and Unwinding

**Opening question:** Why are call stacks sometimes missing or wrong?

**Purpose:** Make profiling failures understandable instead of mysterious.

**Introduces:** symbol table, debug information, stripped binary, stack unwinding, frame pointers, DWARF, last branch records, JIT symbols, kernel symbols.

**Worked examples:**

- a stripped binary with unresolved addresses;
- a frame-pointer-omitted build with broken call chains;
- a JIT runtime requiring its own symbol integration;
- containers whose symbols are not available on the host.

**Guided lab:** Build the same workload with and without frame pointers and debug symbols. Compare profile readability.

**Policy:** Do not mandate frame pointers universally. Explain the trade-off and the available unwinding methods.

**Key takeaway:** **A profiler can only reconstruct the call path from the metadata and unwinding evidence the build and runtime make available.**

**Next question:** How can thousands of sampled call stacks be read as one coherent picture?

---

## Chapter 14 — Flame Graphs: Reading the Shape of Work

**Opening question:** How do flame graphs show the shape of work?

**Purpose:** Teach flame graphs correctly, including what width, height, ordering, and color do and do not mean.

**Introduces:** folded stack, stack aggregation, frame width, ancestry, plateau, tower, CPU flame graph, off-CPU flame graph.

**Canonical generation path:**

```bash
perf record -F 99 -g -- ./cyclelab mixed
perf script > out.perf
stackcollapse-perf.pl out.perf > out.folded
flamegraph.pl out.folded > flame.svg
```

The exact path may be supplemented with newer integrated tools, but the capture–fold–render model must remain explicit.

**Mandatory misconceptions:**

- vertical height means elapsed time;
- left-to-right order is chronological;
- the hottest color is the hottest function;
- the widest leaf is automatically waste;
- a narrow rare path cannot matter to tail latency.

**Guided lab:** Read a prepared flame graph before generating one. The reader must identify:
- broad base frames;
- leaf-heavy CPU work;
- wrapper functions;
- separate execution towers;
- missing symbols.

**Key takeaway:** **A flame graph compresses sampled stacks into width: it shows where execution accumulated in the call hierarchy, not a timeline.**

**Next question:** How do we compare a profile before and after a change without fooling ourselves?

---

## Chapter 15 — Differential Profiling and Optimization Proof

**Opening question:** How do we prove that an optimization changed the right thing?

**Purpose:** Connect profiles back to benchmarking discipline.

**Introduces:** before/after profile, normalized workload, differential flame graph, `perf diff`, regression, bottleneck shift, total-work normalization.

**Worked example:** An optimization removes 30% of samples from one function but total throughput barely changes because the saved time moves to memory allocation and lock contention.

**Guided lab:**

1. benchmark baseline;
2. capture baseline profile;
3. apply one code change;
4. rerun in interleaved order;
5. capture new profile under equal completed work;
6. compare elapsed time, throughput, counters, and stack shape;
7. inspect new bottleneck.

**Mandatory warning:** Comparing profiles collected for equal duration can be misleading when the faster version completes more work. State whether profiles are normalized by time, requests, iterations, or another unit.

**Key takeaway:** **An optimization is proven by a better workload outcome under controlled conditions, while profiles explain how the work changed.**

**Next question:** What if the hot path spends most of its time waiting for data rather than executing arithmetic?

---

# Part IV — Why Memory Changes Everything

## Chapter 16 — The Memory Hierarchy and Locality

**Opening question:** Why can memory access dominate code that performs little computation?

**Purpose:** Build the hierarchy model required for caches, bandwidth, and NUMA.

**Introduces:** register, cache line, L1, L2, last-level cache, DRAM, latency, locality, spatial locality, temporal locality.

**Canonical analogy:** A desk, nearby drawers, a room archive, and a remote warehouse—useful only if the text immediately explains where the analogy breaks.

**Guided lab:** Traverse the same array sequentially and randomly over increasing sizes. Plot time per access against working-set size.

**Mandatory distinction:** Cache capacity, cache latency, and memory bandwidth are related but different.

**Key takeaway:** **Memory performance depends not only on how much data is used, but on when, where, and in what order it is accessed.**

**Next question:** Why do working-set size and access pattern create sudden performance changes?

---

## Chapter 17 — Working Sets, Cache Misses, and Prefetching

**Opening question:** Why do access order and working-set size change performance?

**Purpose:** Explain misses through reuse distance and hardware prefetch behavior rather than through a single miss-rate number.

**Introduces:** working set, cache hit, compulsory miss, capacity miss intuition, stride, reuse, prefetcher, TLB as a forward pointer.

**Worked examples:**

- row-major versus column-major traversal;
- sequential versus pointer-chasing access;
- stride sizes that defeat or assist prefetching;
- a small working set shared by many threads.

**Guided lab:** Sweep array size and stride. Collect elapsed time and carefully selected cache/TLB events when available.

**Misconceptions:**

- a high cache-miss percentage always means poor performance;
- a low miss percentage means memory is not the bottleneck;
- prefetching makes random access sequential;
- cache events are portable across all CPUs.

**Key takeaway:** **Caches reward reuse and predictable access; miss counts become meaningful only when connected to access volume, latency, and completed work.**

**Next question:** How can threads that touch different variables still slow each other down?

---

## Chapter 18 — Cache Coherence and False Sharing

**Opening question:** How can independent threads slow each other through cache coherence?

**Purpose:** Explain that coherence operates on cache lines, not language-level variables.

**Introduces:** shared cache line, ownership, invalidation, coherence traffic, true sharing, false sharing, HITM intuition.

**Canonical example:** Per-thread counters placed next to each other in one structure.

**Guided lab:** Run `cyclelab false-sharing` with packed counters and padded counters. Measure scaling across thread counts. Use `perf c2c` where supported, while providing a tool-independent interpretation path.

**Mandatory cautions:**

- padding can increase memory footprint and hurt locality;
- not every shared line is false sharing;
- false sharing may appear only at certain thread counts or topologies;
- hardware support for precise cache-to-cache analysis varies.

**Key takeaway:** **Threads can contend through the cache-coherence protocol even when the source code says they own different variables.**

**Next question:** When individual accesses are efficient, can the system still run out of memory throughput?

---

## Chapter 19 — Memory Bandwidth and the Roofline Intuition

**Opening question:** How do we tell whether a workload is limited by memory bandwidth?

**Purpose:** Distinguish latency-bound, bandwidth-bound, and compute-bound behavior.

**Introduces:** bytes transferred, sustained bandwidth, peak bandwidth, arithmetic intensity, memory-level parallelism, bandwidth saturation, roofline intuition.

**Worked examples:**

- pointer chasing: low bandwidth but latency-bound;
- streaming copy: high bandwidth and bandwidth-bound;
- compute-heavy loop: little memory traffic relative to work;
- multithreaded scaling that stops when memory channels saturate.

**Guided lab:** Run a streaming kernel at increasing thread counts. Plot throughput and measured memory bandwidth. Identify where more threads stop helping.

**Policy:** The roofline model is used as a mental model, not as a full HPC modeling chapter.

**Key takeaway:** **A workload is bandwidth-bound when useful work cannot increase because the memory system is already moving data near its sustainable rate.**

**Next question:** Which Linux and vendor tools can reveal memory behavior directly?

---

## Chapter 20 — Measuring Memory with `perf mem`, `perf c2c`, and PCM

**Opening question:** Which tools reveal cache and memory behavior?

**Purpose:** Provide a layered workflow rather than one universal memory command.

**Tool ladder:**

1. controlled access-pattern experiments;
2. `perf stat` with architecture-appropriate events;
3. `perf mem` for sampled memory accesses where supported;
4. `perf c2c` for shared-line and false-sharing analysis;
5. uncore PMUs and vendor tools such as Intel PCM for socket/channel bandwidth;
6. application counters and source-level instrumentation.

**Introduces:** load/store sampling, data source, local/remote classification, cache-to-cache transfer, uncore, memory controller.

**Guided labs:**

- use `perf mem` on sequential and random access;
- use `perf c2c` on packed and padded counters;
- use a supported memory-bandwidth tool during the streaming benchmark.

Each lab must include a fallback when the hardware, hypervisor, kernel, or permissions do not expose the required events.

**Key takeaway:** **Memory analysis is a chain of partial observations; no single counter reports “the memory bottleneck” for every machine.**

**Next question:** Memory behavior depends on where threads run—how does Linux choose that location?

---

# Part V — Where the Work Runs

## Chapter 21 — The Scheduler, Run Queues, and CPU Time

**Opening question:** How does Linux decide where runnable work executes?

**Purpose:** Build a practical scheduler model without becoming a scheduler-internals book.

**Introduces:** runnable, running, sleeping, run queue, time slice intuition, wake-up, load balancing, scheduling class as a boundary.

**Worked example:** Four CPU-bound threads on two CPUs versus two CPU-bound and two sleeping threads.

**Guided lab:** Use process-state observation, `pidstat`, and selected `perf sched` views to compare runnable pressure and actual execution.

**Mandatory distinction:** CPU utilization does not directly show how long a runnable thread waited before running.

**Key takeaway:** **A thread consumes CPU only while running, but its latency can grow while it waits runnable in a queue.**

**Next question:** What costs arise when work moves or competes with unrelated work?

---

## Chapter 22 — Context Switches, Migrations, and Interference

**Opening question:** How do migrations and neighboring workloads add noise and delay?

**Purpose:** Explain scheduler-induced variability and shared-resource interference.

**Introduces:** voluntary context switch, involuntary context switch, migration, cache warmth, interrupt, steal time, noisy neighbor, CPU pressure.

**Worked examples:**

- a thread waking frequently and losing cache locality;
- a benchmark sharing a core with background work;
- a virtual machine losing physical CPU time;
- interrupts concentrated on one CPU.

**Guided lab:** Run the same benchmark alone and with a controlled interfering workload. Inspect context switches, migrations, CPU placement, and variance.

**Misconception:** Context switches are always the root cause when their count is high.

**Key takeaway:** **Scheduling events matter when they delay critical work or destroy useful locality; their counts alone do not prove harm.**

**Next question:** Can fixing work to chosen CPUs remove that variability?

---

## Chapter 23 — CPU Affinity, SMT, cpusets, and Isolation

**Opening question:** When does CPU affinity help, hurt, or merely hide a problem?

**Purpose:** Teach placement as an experimental control and workload policy, not a universal optimization.

**Introduces:** logical CPU, physical core, SMT sibling, socket, affinity mask, `taskset`, `sched_setaffinity`, cpuset, cgroup CPU constraints, isolation.

**Guided experiments:**

- pin one thread and compare variance;
- place two CPU-heavy threads on SMT siblings versus separate cores;
- pin a memory-heavy workload within one socket;
- demonstrate that pinning too many runnable threads to too few CPUs increases queueing.

**Mandatory cautions:**

- affinity can block scheduler load balancing;
- CPU affinity does not automatically bind memory;
- container CPU limits and cpusets can override expectations;
- `isolcpus` and low-latency boot tuning are advanced operational choices, not first-line fixes.

**Key takeaway:** **Affinity trades scheduler freedom for placement control; it helps only when the chosen topology matches the workload.**

**Next question:** If a thread is pinned to one socket, where is its memory physically allocated?

---

## Chapter 24 — NUMA: Distance Inside One Machine

**Opening question:** Why is some memory farther away than other memory?

**Purpose:** Build the NUMA model from hardware topology and scalable memory bandwidth.

**Introduces:** NUMA node, local memory, remote memory, socket, interconnect, distance, node topology, memory-only node as an advanced note.

**Worked example:** A two-socket machine where a thread runs on socket 1 while most pages were allocated on socket 0.

**Guided lab:** Inspect topology with:

```bash
lscpu
lscpu -e
numactl --hardware
cat /sys/devices/system/node/node*/distance
```

On a single-node machine, use schematic data or optional virtual NUMA only as a teaching aid, not as equivalent performance evidence.

**Misconception:** NUMA is only an issue on extremely large supercomputers.

**Key takeaway:** **NUMA makes memory placement part of execution placement: the same address can cost more depending on which CPU accesses it.**

**Next question:** What determines where pages are allocated, and how can we control it?

---

## Chapter 25 — First-Touch, Memory Placement, and NUMA Diagnosis

**Opening question:** How do allocation and execution placement interact on NUMA systems?

**Purpose:** Turn the NUMA model into a safe diagnostic workflow.

**Introduces:** first-touch allocation, local allocation policy, interleave, bind, preferred node, automatic NUMA balancing, page migration, NUMA hit/miss statistics.

**Worked examples:**

- one thread initializes an array, many threads process it elsewhere;
- interleaving improves aggregate bandwidth but can worsen single-thread latency;
- CPU pinning without memory placement causes remote accesses;
- moving memory fixes locality but creates migration cost.

**Guided lab:**

```bash
numactl --cpunodebind=0 --membind=0 ./cyclelab numa
numactl --cpunodebind=1 --membind=0 ./cyclelab numa
numactl --interleave=all ./cyclelab bandwidth
numastat -p <pid>
```

Exact commands must be conditional on machine topology.

**Key takeaway:** **NUMA performance follows the relationship between where pages are first placed and where the threads that use them actually run.**

**Next question:** Counters and profiles still miss many transient events—how can we observe live kernel behavior?

---

# Part VI — Seeing the Invisible

## Chapter 26 — Events, Tracepoints, Kprobes, and Uprobes

**Opening question:** What can dynamic tracing observe that counters and sampling cannot?

**Purpose:** Introduce event-driven instrumentation before eBPF implementation details.

**Introduces:** event, tracepoint, function entry/return, kprobe, kretprobe, uprobe, USDT, argument capture, event rate.

**Worked example:** Measuring a latency distribution between function entry and return cannot be derived from a simple event count.

**Tool ladder:** Existing stable tracepoint first; then USDT; then function probe only when necessary.

**Mandatory cautions:**

- internal kernel functions can change;
- probe names and arguments are version-dependent;
- high-frequency probes can produce large overhead;
- tracing every event is not inherently better than sampling;
- data captured from processes may be sensitive.

**Key takeaway:** **Dynamic tracing attaches measurement to specific events, allowing questions that aggregate counters and periodic samples cannot answer.**

**Next question:** How can custom tracing logic execute safely in a running kernel?

---

## Chapter 27 — The eBPF Mental Model

**Opening question:** How can eBPF safely run custom measurements inside the kernel?

**Purpose:** Explain eBPF without beginning with syntax.

**Introduces:** eBPF program, hook, verifier, helper, map, per-CPU map, ring buffer, user-space loader, BTF, CO-RE.

### Core execution model

1. user space defines or loads a program;
2. the kernel verifier checks allowed control flow and memory access;
3. the program attaches to a supported hook;
4. the hook triggers the program;
5. the program aggregates data in maps or emits selected events;
6. user space reads and presents the results.

**Worked example:** Instead of printing every block-I/O completion, an eBPF program stores a latency histogram in a map and emits only the aggregate.

**Mandatory misconceptions:**

- eBPF is a background daemon;
- eBPF can safely execute arbitrary kernel code;
- eBPF is always zero-overhead;
- CO-RE makes every program portable to every kernel;
- maps are ordinary user-space hash maps.

**Key takeaway:** **eBPF is a constrained in-kernel execution model that turns selected events into bounded measurements and aggregates.**

**Next question:** How can an engineer use that model without writing a full libbpf application?

---

## Chapter 28 — `bpftrace` and BCC: Questions as Programs

**Opening question:** How do `bpftrace` and BCC turn questions into live instrumentation?

**Purpose:** Give the reader a practical entry point while preserving the underlying model.

**Introduces:** probe specification, predicate, action, aggregation, histogram, stack aggregation, interval output.

**Progression:**

1. list available probes;
2. count an event;
3. group by process or thread;
4. measure a duration;
5. build a histogram;
6. capture user or kernel stacks;
7. use a packaged BCC tool before writing a custom script.

**Example questions:**

- Which processes call this syscall most?
- What is the distribution of block-I/O latency?
- How long do threads wait to be scheduled after wake-up?
- Which user-space stacks allocate memory?
- Which locks or futex waits dominate blocked time?

**Tool choice policy:**

- use existing BCC tools for standard investigations;
- use `bpftrace` for short, inspectable questions;
- use libbpf/CO-RE for maintained production tooling;
- explain that package names and tool suffixes vary by distribution.

**Key takeaway:** **`bpftrace` and BCC are most powerful when the question is stated precisely enough to become an event, a key, and an aggregate.**

**Next question:** How do we explain time that CPU profiles cannot see?

---

## Chapter 29 — Off-CPU Time, I/O Latency, and Contention

**Opening question:** Where does time go when a thread is not on a CPU?

**Purpose:** Complete the time-accounting model.

**Introduces:** off-CPU time, blocked stack, wake-up latency, sleep, futex wait, lock contention, block I/O, queueing delay, off-CPU flame graph.

**Worked examples:**

- a thread blocks on a mutex;
- a request waits in a run queue after being woken;
- a synchronous read waits on storage;
- a thread sleeps intentionally;
- an asynchronous runtime parks work while another task runs.

**Guided labs:**

- run a packaged run-queue latency tool;
- collect an off-CPU profile for lock contention;
- collect a block-I/O latency histogram;
- compare on-CPU and off-CPU flame graphs for the same workload.

**Mandatory warning:** Off-CPU time is not automatically waste. Waiting may be correct, intentional, or imposed by an external dependency.

**Key takeaway:** **A complete latency explanation joins on-CPU execution with the reasons, locations, and durations for which critical threads were not running.**

**Next question:** How do we combine workload design, counters, profiles, memory analysis, topology, and tracing into one investigation?

---

## Chapter 30 — The Complete Linux Performance Investigation

**Opening question:** How do all the layers combine into one defensible investigation?

**Purpose:** Synthesize the entire book through one realistic case study.

### Case study shape

A multithreaded service shows:

- acceptable average CPU utilization;
- degraded p99 latency;
- inconsistent benchmark results;
- weak scaling after four workers;
- occasional long stalls.

The investigation should reveal several layers, but only one at a time:

1. the benchmark is initially invalid because request rate and warm-up are uncontrolled;
2. corrected measurements show saturation and tail growth;
3. CPU profiles reveal a broad serialization path;
4. off-CPU tracing shows lock waiting;
5. after fixing contention, throughput rises but scaling stops at memory bandwidth;
6. thread placement reveals cross-socket memory traffic;
7. first-touch initialization restores locality;
8. the final benchmark proves improvement and documents remaining limits.

### Final report template

- workload and success metric;
- environment;
- baseline distribution;
- initial observations;
- hypotheses considered;
- measurements and overhead;
- changes made;
- before/after evidence;
- bottleneck movement;
- limitations;
- rollback or safety considerations;
- next experiment.

**Key takeaway:** **Good performance engineering is a chain of bounded claims, each supported by a measurement that was chosen to distinguish competing explanations.**

---

## 13. Laboratory Environment

### 13.1 Reference environment

The reference environment should be a recent Linux distribution with:

- a kernel whose matching `perf` tools are available;
- at least four logical CPUs;
- preferably more than one physical core;
- at least 8 GB of memory;
- debug symbols for the lab binaries;
- root or delegated observability capabilities for selected exercises;
- bare metal preferred.

A dual-socket NUMA machine is ideal for Chapters 24–25 but must not be required to read the book.

### 13.2 Virtualization policy

Virtual machines and cloud instances are useful but have limitations:

- PMU events may be hidden, incomplete, or virtualized;
- cycle counts may behave differently;
- steal time and host interference add noise;
- NUMA topology may be synthetic;
- uncore and memory-controller counters may be unavailable;
- eBPF features depend on the host kernel and security policy.

The book must mark labs as:

- **portable** — expected on most Linux systems;
- **hardware-dependent** — requires a compatible PMU or topology;
- **privileged** — requires capabilities or elevated permissions;
- **bare-metal recommended** — virtualization may invalidate the lesson.

### 13.3 Environment doctor

The repository should include:

```bash
make doctor
```

It should report:

- kernel and distribution;
- CPU model and architecture;
- logical CPU/core/socket topology;
- NUMA nodes and distances;
- current CPU frequency/governor information where available;
- `perf` version and matching-kernel status;
- `perf_event_paranoid`;
- `kptr_restrict`;
- BTF availability;
- `bpftrace` and BCC availability;
- FlameGraph scripts;
- supported PMU events;
- whether the session appears virtualized;
- optional Intel PCM availability;
- warnings, not silent failures.

---

## 14. Benchmarking Hygiene Checklist

Every benchmark chapter and case study should use this checklist.

### Define

- What exact workload is being measured?
- What input data is used?
- What is the primary metric?
- What secondary metrics protect against regressions?
- Is the question about cold or warm behavior?
- Is the question about one operation, sustained throughput, or tail latency?

### Control

- Keep software versions and build flags fixed.
- Record kernel, CPU, topology, and machine state.
- Separate setup from measured work.
- Pin CPU or memory only when placement is part of the question.
- Control background load where possible.
- Watch thermal and frequency changes.
- Prevent compiler elimination and validate output.
- Use the same completed work for before/after comparisons.

### Repeat

- Warm up when the production workload is warm.
- Run enough repetitions to observe the distribution.
- Interleave or randomize variants.
- Retain raw results.
- Report median and relevant percentiles.
- Show variability and outliers rather than deleting inconvenient runs without explanation.

### Validate

- Confirm the benchmark exercises the intended path.
- Check that profiling does not materially change behavior.
- Check for event multiplexing and unsupported counters.
- Confirm symbols and stacks are valid.
- Repeat the result on another day or machine when the decision is important.
- Test a realistic workload before generalizing from a microbenchmark.

### Communicate

- State what the data supports.
- State what it does not support.
- Separate observation, interpretation, and recommendation.
- Include enough commands and configuration for reproduction.
- Avoid false precision.

---

## 15. Tool Selection Ladder

The book should repeatedly reinforce the least-invasive-tool principle.

### Level 1 — Workload and basic time

- application metrics;
- `/usr/bin/time`;
- process CPU time;
- request latency and throughput;
- system load and pressure.

### Level 2 — Broad system observation

- `ps`;
- `pidstat`;
- `vmstat`;
- `iostat`;
- `mpstat`;
- pressure stall information where appropriate.

These are context tools, not the main subject.

### Level 3 — Performance counters

- `perf stat`;
- architecture-specific PMU events;
- uncore and vendor counters.

### Level 4 — CPU sampling

- `perf record`;
- `perf report`;
- `perf annotate`;
- flame graphs.

### Level 5 — Specialized memory and scheduler tools

- `perf mem`;
- `perf c2c`;
- `perf sched`;
- NUMA tools;
- memory-bandwidth tools.

### Level 6 — Dynamic tracing

- tracepoints;
- BCC tools;
- `bpftrace`;
- custom libbpf/CO-RE tools.

A chapter must explain why moving down the ladder is justified.

---

## 16. Misconception Registry Seed

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
| M20 | A profiler’s output is ground truth. | Every profiler is a measurement system with scope, overhead, permissions, and blind spots. |

---

## 17. Analogy Registry Seed

Analogies should be reused consistently and retired when they stop helping.

| Concept | Canonical analogy | Constraint |
|---|---|---|
| Latency vs throughput | Travel time for one vehicle vs vehicles per hour | Do not imply roads perfectly model queues. |
| Saturation | Checkout line whose queue grows after cashiers reach capacity | Pair with measured latency curves. |
| CPU pipeline | Multi-stage assembly line with out-of-order readiness | Clarify that CPU execution is more dynamic than a literal conveyor belt. |
| Sampling | Periodically photographing a factory floor | Emphasize statistical representation, not complete history. |
| Flame graph | A population map of observed call stacks | Never call it a timeline. |
| Cache hierarchy | Desk, drawer, archive, warehouse | Use only to establish distance and capacity. |
| Branch prediction | Choosing a route before reaching a fork | Explain recovery cost after the analogy. |
| False sharing | Two people repeatedly erasing separate fields on one shared whiteboard | Emphasize cache-line granularity. |
| Memory bandwidth | Lanes carrying bytes per second | Distinguish lane capacity from trip latency. |
| CPU affinity | Assigning a worker to a workstation | Explain lost flexibility. |
| NUMA | Multiple workshops with local storerooms connected by a corridor | Keep CPU placement and memory placement distinct. |
| eBPF | Temporary sensors attached to defined points in a running machine | Sensors are constrained programs, not passive magic. |
| Benchmark | Controlled scientific experiment | Use literally: hypothesis, controls, repetitions, uncertainty. |

---

## 18. Style Guide

### Voice

- Write to a capable engineer, not to a novice child and not to a CPU architect.
- Use precise, concrete sentences.
- Introduce the plain-language mechanism before the technical term.
- Use “may,” “can,” and “on this system” when hardware support varies.
- Distinguish facts, hypotheses, and heuristics.
- Never call a result “obvious” before the reader has the model.
- Never shame the reader for having trusted a common metric.
- Avoid performance folklore and macho optimization language.

### Commands

- Commands must be copyable.
- Mark placeholders clearly.
- Explain required privileges.
- Explain process-wide, thread-wide, CPU-wide, and system-wide scope.
- Include cleanup steps.
- Avoid pipelines that silently discard errors.
- Show how to save raw data.
- Prefer commands that are available in upstream tools.
- State when output differs by CPU, kernel, or distribution.

### Numbers

- Use measured numbers only with environment provenance.
- Use rounded illustrative numbers when teaching a concept.
- Never compare percentages whose denominators differ.
- Never claim a universal threshold for “good IPC,” “bad miss rate,” or “high context-switch count.”
- Use distributions for latency and repeated benchmarks.

### Architecture portability

The main prose should remain architecture-neutral. Put x86, Arm64, Intel, AMD, and vendor-specific details in:

- sidebars;
- lab notes;
- tool-availability boxes;
- appendices.

The manuscript must not use one Intel event name as though it were a universal CPU concept.

---

## 19. Source Policy

### Preferred sources

1. Linux kernel documentation and source-tree documentation;
2. Linux `perf` manual pages and upstream perf wiki;
3. upstream project documentation for FlameGraph, BCC, `bpftrace`, and libbpf;
4. processor-vendor optimization manuals and PMU documentation;
5. original research papers for hardware or measurement techniques;
6. primary documentation for benchmark frameworks.

### Rules

- Every hardware-specific event description must be checked against the relevant architecture documentation.
- Every command must be tested against a recorded tool version.
- Blog posts may provide incidents or teaching ideas, but technical claims must be anchored in primary sources.
- Historical claims must be cited.
- Outputs copied from a real machine must include environment metadata.
- The book should prefer stable concepts over transient command syntax.
- Tool limitations and permissions are part of the explanation, not footnotes.

### Initial technical source backbone

- Linux perf documentation: <https://docs.kernel.org/admin-guide/perf/index.html>
- Linux workload tracing guide: <https://docs.kernel.org/admin-guide/workload-tracing.html>
- perf tutorial: <https://perfwiki.github.io/main/tutorial/>
- perf manual pages: <https://man7.org/linux/man-pages/man1/perf.1.html>
- FlameGraph project: <https://github.com/brendangregg/FlameGraph>
- Linux BPF documentation: <https://docs.kernel.org/bpf/>
- libbpf overview: <https://docs.kernel.org/bpf/libbpf/libbpf_overview.html>
- bpftrace documentation: <https://bpftrace.org/docs/release_026/docs>
- BCC project and tools: <https://github.com/iovisor/bcc>
- Linux false-sharing documentation: <https://docs.kernel.org/kernel-hacking/false-sharing.html>
- Linux NUMA overview: <https://docs.kernel.org/mm/numa.html>
- Linux NUMA memory policy: <https://docs.kernel.org/admin-guide/mm/numa_memory_policy.html>
- `numactl` manual: <https://man7.org/linux/man-pages/man8/numactl.8.html>
- CPU affinity API: <https://man7.org/linux/man-pages/man2/sched_setaffinity.2.html>
- Intel Performance Counter Monitor: <https://github.com/intel/pcm>
- Google Benchmark user guide: <https://google.github.io/benchmark/user_guide.html>

The final bibliography should add architecture-specific and chapter-specific sources.

---

## 20. Repository Structure

```text
thinking-in-cycles/
├── README.md
├── BLUEPRINT.md
├── ROADMAP.md
├── CONTRIBUTING.md
├── LICENSE
├── style-guide.md
├── glossary.md
├── misconceptions.md
├── analogy-registry.md
├── concept-graph.yaml
├── concept-graph.md
├── book/
│   ├── README.md
│   ├── preface.md
│   ├── about-the-author.md
│   ├── part-1-measuring-reality/
│   ├── part-2-what-the-cpu-is-doing/
│   ├── part-3-where-cpu-time-goes/
│   ├── part-4-why-memory-changes-everything/
│   ├── part-5-where-the-work-runs/
│   ├── part-6-seeing-the-invisible/
│   └── appendices/
├── labs/
│   ├── cyclelab/
│   ├── mini-service/
│   ├── scripts/
│   ├── expected-shapes/
│   └── datasets/
├── figures/
│   ├── source/
│   └── generated/
├── references/
│   ├── bibliography.md
│   └── chapters/
├── templates/
│   ├── chapter-template.md
│   ├── lab-template.md
│   └── performance-report-template.md
├── scripts/
│   ├── doctor.sh
│   ├── validate_concept_graph.py
│   ├── validate_chapter_metadata.py
│   ├── validate_links.py
│   ├── smoke_test_labs.sh
│   └── prepare_manuscript_for_publish.py
└── publish/
    ├── _quarto.yml
    └── ...
```

---

## 21. Validation Rules

Automated validation should check:

- chapter numbering and filenames;
- part and chapter order;
- concept prerequisites;
- glossary terms introduced before use;
- misconception IDs;
- internal links;
- referenced lab paths;
- shell syntax where practical;
- lab binaries compile;
- commands marked as architecture-specific;
- figures have captions and source metadata;
- every chapter has one key takeaway and one next question;
- Chapter N’s next question matches Chapter N+1’s opening question.

### What CI must not do

CI must not fail a build because a benchmark did not reach a performance threshold. Shared runners are not valid performance laboratories.

CI may:

- compile labs;
- run correctness checks;
- execute short smoke tests;
- validate output schemas;
- verify that benchmark commands terminate;
- ensure checksums match.

---

## 22. Appendices

### Appendix A — Building a Trustworthy Lab Machine

- bare metal versus VM;
- matching `perf` to the kernel;
- debug symbols;
- permissions and capabilities;
- CPU governors and turbo;
- thermal monitoring;
- background services;
- safe use of root.

### Appendix B — `perf` Command Map

A compact question-to-command guide, not a replacement for the main narrative.

### Appendix C — Hardware Event Portability

- generic events;
- raw events;
- event aliases;
- PMU models;
- multiplexing;
- hybrid CPUs;
- precise event-based sampling;
- Intel PEBS, AMD IBS, Arm SPE as architecture notes.

### Appendix D — Runtime-Specific Profiling Notes

Separate short sections for:

- C/C++;
- Rust;
- Go;
- JVM;
- Python;
- JIT symbolization;
- frame pointers and runtime profilers.

### Appendix E — Containers and cgroups

- namespace versus host visibility;
- cgroup-scoped perf;
- cpusets and quotas;
- eBPF from host versus container;
- symbol paths;
- Kubernetes limitations.

### Appendix F — Statistical Reasoning for Benchmarks

- medians and percentiles;
- confidence intervals;
- bootstrap intuition;
- effect size;
- repeated comparisons;
- multiple testing;
- practical versus statistical significance.

### Appendix G — Production Safety

- scope probes narrowly;
- bound duration and output;
- prefer aggregation;
- estimate overhead;
- protect sensitive data;
- secure performance tooling;
- retain rollback paths;
- communicate operational risk.

---

## 23. Editorial Invariants

The following must remain true through every revision:

1. The book is organized by questions, not tools.
2. Every specialized tool appears only after the phenomenon it measures.
3. Every optimization is presented as conditional.
4. Every benchmark result includes provenance.
5. Hardware-specific claims are marked as hardware-specific.
6. A counter never becomes a verdict by itself.
7. CPU profiles are never presented as complete latency profiles.
8. CPU affinity and memory affinity are never conflated.
9. eBPF overhead and safety constraints are always discussed.
10. Exact numeric outputs are never promised across machines.
11. The reader repeatedly predicts results before measuring.
12. The final case study uses the full investigation loop.
13. The manuscript teaches how to stop investigating when evidence is sufficient.
14. “Faster” always means faster for a stated workload and metric.
15. Correctness is never traded for speed without being stated explicitly.

---

## 24. Drafting Order

Draft by concept dependency, not by excitement about tools.

### Phase 1 — Foundation

- Chapters 1–5;
- benchmark harness;
- environment doctor;
- glossary and misconception registry.

### Phase 2 — CPU model

- Chapters 6–10;
- compute and branch modes;
- architecture-review pass.

### Phase 3 — Profiling

- Chapters 11–15;
- symbol and unwinding fixtures;
- flame-graph generation pipeline.

### Phase 4 — Memory

- Chapters 16–20;
- sequential, random, bandwidth, and false-sharing modes;
- multi-architecture event audit.

### Phase 5 — Topology

- Chapters 21–25;
- affinity and NUMA labs;
- single-node fallback material.

### Phase 6 — Tracing and synthesis

- Chapters 26–30;
- BCC and `bpftrace` labs;
- full service case study;
- production-safety review.

### Phase 7 — Editorial integration

- dependency validation;
- analogy consistency;
- command testing;
- bibliography;
- technical review by:
  - a Linux perf practitioner;
  - an eBPF practitioner;
  - a CPU/memory specialist;
  - an engineer from the intended audience.

---

## 25. Definition of Done for a Chapter

A chapter is complete only when:

- its opening question is answerable in one sentence;
- prerequisites are respected;
- the story and worked example are distinct;
- the reader predicts something before measuring;
- the core mechanism is technically correct;
- all commands have been tested or clearly marked schematic;
- expected outcomes are qualitative, not machine-specific;
- permissions and overhead are explained;
- hardware limitations are stated;
- at least two misconceptions are addressed where relevant;
- the guided lab has a fallback path;
- raw data or figure-generation instructions are retained;
- new glossary terms are added;
- misconception entries are added;
- analogy use is registered;
- references are recorded;
- the key takeaway is one sentence;
- the next question exactly leads into the next chapter.

---

## 26. Definition of Done for the Book

The first complete edition is ready when:

- all 30 chapters are written;
- every lab compiles and passes correctness smoke tests;
- the book can be read without access to specialized hardware;
- readers with supported hardware can reproduce the qualitative lab results;
- x86-64 and Arm64 limitations are reviewed;
- every architecture-specific event is clearly isolated;
- the full case study is reproducible;
- all performance claims have provenance;
- the concept graph has no forward-reference violations;
- the glossary and misconception registry are complete;
- the HTML, PDF, and EPUB builds succeed;
- at least three target readers complete a reading-and-lab pass;
- at least two domain experts complete technical review;
- feedback has produced a documented revision pass;
- the manuscript can answer its central promise:

> Given a slow Linux workload, can the reader explain where the time went and produce evidence for what to change next?

---

## 27. One-Paragraph Book Description

*Thinking in Cycles* is a practical, intuition-first guide to Linux performance engineering. It builds a cumulative model of how workloads become processes and threads, how Linux schedules them, how CPUs execute instructions, how caches and memory shape performance, how topology and NUMA affect placement, and how tools such as `perf`, flame graphs, BCC, and `bpftrace` reveal different parts of the system. Rather than teaching a collection of commands, it teaches readers to define a workload, form a hypothesis, select the least invasive measurement, interpret the evidence, and prove whether an optimization actually helped.

---

## 28. Final Promise to the Reader

By the end of this book, “the system is slow” should no longer feel like a mysterious complaint.

It should feel like the beginning of a structured investigation.
