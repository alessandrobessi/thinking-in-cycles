# Glossary

Terms are grouped by concept level (see `concept-graph.md`), alphabetical
within each group, then a **Supplementary vocabulary** section for teaching
terms Chapters 1–5 use that BLUEPRINT.md Section 11 doesn't formally list at
any level. Status as of this revision: Chapters 1–5 drafted.

## Level 0 — Questions and Measurements

### baseline
**First introduced:** Chapter 5
A controlled reference measurement taken before a change, used to judge
whether a later measurement differs meaningfully.
**See also:** experiment, confounder

### confounder
**First introduced:** Chapter 4
A factor other than the one being tested that could account for an observed
difference between two measurements.
**See also:** randomization, baseline

### distribution
**First introduced:** Chapter 4
The full spread of repeated measurements, not just their average — the
shape a benchmark's results take across many runs.
**See also:** variance, outlier, repetition

### experiment
**First introduced:** Chapter 5 (used informally from Chapter 1 onward)
A controlled, repeatable measurement designed to test one hypothesis at a
time.
**See also:** hypothesis, baseline, confounder

### latency
**First introduced:** Chapter 3
The elapsed time to complete one unit of work, measured from request to
response.
**See also:** throughput, tail latency

### measurement overhead
**First introduced:** not yet (first full treatment: Chapter 11)
The cost, in time or resources, that the act of measuring adds to the thing
being measured.
**See also:** perturbation

### metric
**First introduced:** not yet formally named, though it is Chapter 2's whole subject
A specific, numeric definition of what "better" means for a workload.
**See also:** workload, service level

### saturation
**First introduced:** Chapter 3
The point past which additional concurrent work mostly increases waiting
rather than completed work.
**See also:** utilization, queue, concurrency

### throughput
**First introduced:** Chapter 3
The amount of completed work per unit of elapsed time.
**See also:** latency, capacity

### utilization
**First introduced:** Chapter 3
The fraction of available time a resource spent busy.
**See also:** saturation

### variance
**First introduced:** Chapter 4
How much repeated measurements of the same thing differ from one another.
**See also:** distribution, outlier

### workload
**First introduced:** Chapter 1 (formalized in Chapter 2 as "workload model")
A defined, reproducible unit of work whose performance is being studied —
never an abstract "the program."
**See also:** operation, workload model

## Level 1 — Linux Execution

None of these are formally introduced yet (first full treatment: Part V,
Chapters 21–23). Chapter 1's guided lab uses **user time**, **system time**,
and **wall time** operationally, comparing them for a `cyclelab compute`
run, without a formal scheduler model.

### blocked
A thread state in which a thread cannot run because it is waiting on an
event (I/O, a lock, a signal) rather than merely waiting for a CPU.

### context switch
The act of a CPU stopping execution of one thread and beginning another,
saving and restoring the state needed to resume each.

### CPU migration
A thread resuming execution on a different logical CPU than the one it
last ran on.

### process
An OS-managed unit of execution with its own address space, containing one
or more threads.

### run queue
The set of runnable threads waiting for a CPU on which to execute.

### runnable
A thread state in which a thread is ready to execute but is not currently
assigned a CPU.

### running
A thread state in which a thread is currently executing on a CPU.

### scheduler
The kernel subsystem that decides which runnable thread runs on which CPU
and for how long.

### sleeping
A thread state in which a thread has voluntarily given up the CPU to wait
for a condition, timer, or event.

### system time
**Used informally in:** Chapter 1
CPU time spent executing kernel code on behalf of a process (e.g. handling
a system call).

### thread
The unit of scheduling and execution within a process; a process may
contain many threads sharing one address space.

### user time
**Used informally in:** Chapter 1
CPU time spent executing a process's own (non-kernel) instructions.

### wall time
**Used informally in:** Chapter 1
Elapsed real-world time between the start and end of an operation,
regardless of how it was spent.

## Levels 2–7

Not yet introduced by any drafted chapter. Term lists live in
`concept-graph.yaml`/`concept-graph.md`; definitions will be added as
Chapters 6–30 are drafted.

**Exception:** `on-CPU time` and `off-CPU time` (formally Level 7, Chapter
29) are used informally starting in Chapter 1 to establish the book's
central time-accounting problem — see `concept-graph.md`'s "Known tensions"
section.

### on-CPU time
**Used informally in:** Chapter 1 · **Formal definition:** Chapter 29
Time a thread spends actually executing on a CPU.

### off-CPU time
**Used informally in:** Chapter 1 · **Formal definition:** Chapter 29
Time a thread spends not executing on a CPU for any reason — runnable and
waiting, blocked, or sleeping.

## Supplementary vocabulary

Plain teaching terms Chapters 1–5 introduce that Section 11 doesn't list at
any formal level. Tracked here so they're not silently invented; not part
of `concept-graph.yaml`'s level structure.

### Chapter 1

**resource** — Any finite thing a workload can compete for: a CPU, a
cache, memory bandwidth, a lock, a disk, a network link.

**bottleneck** — The resource or step whose limits currently determine how
fast the workload as a whole can go.

**critical path** — The sequence of dependent steps that determines the
minimum possible completion time, because none of its steps can overlap
with each other.

### Chapter 2

**operation** — The smallest unit of work a workload's metric is defined
over (e.g., one request, one record, one batch job).

**workload model** — An explicit description of what work is being
generated, at what rate, and with what input characteristics.

**response time** — The elapsed time from when an operation is requested
to when its result is available.

**completion time** — The elapsed time for an entire body of work (e.g. a
batch job) to finish, from start to last operation done.

**service level** — A stated target for a metric (e.g. "p99 latency under
100 ms") that a system is expected to meet.

**capacity** — The maximum sustainable rate of work a system can complete
while still meeting its service level.

**cost per unit of work** — The resources consumed (CPU-time, dollars,
energy) divided by the amount of useful work completed.

### Chapter 3

**concurrency** — The number of operations in flight at once, being worked
on or waiting to be worked on.

**queue** — A place where work waits before it can be served, whose growth
signals that arrivals now exceed the system's service rate.

### Chapter 4

**warm-up** — Running a workload before measuring it so caches,
allocators, JIT compilers, and other transient state reach a steady
condition representative of production.

**repetition** — Running the same controlled experiment multiple times so
its distribution, not a single sample, can be examined.

**outlier** — A measurement far outside the bulk of a distribution, which
may indicate noise, interference, or a real rare event — not automatically
discardable without explanation.

**randomization** — Running variants of a benchmark in an unpredictable or
interleaved order so time-correlated effects (thermal drift, background
load) don't systematically favor one variant.

**thermal state** — The temperature-driven condition of a CPU, which can
throttle clock frequency and change performance mid-benchmark.

**frequency scaling** — The CPU's or OS's automatic adjustment of clock
frequency in response to load, power, and thermal conditions.

**background interference** — Unrelated work on the same machine that
competes for CPU, cache, memory bandwidth, or I/O during a benchmark.

**confidence interval intuition** — An informal sense that a measured
difference should be judged against how much repeated measurements of the
same thing naturally vary, not treated as exact.

### Chapter 5

**hypothesis** — A specific, testable guess about where time is going or
what a change will do, stated before measuring.

**falsification** — Designing a measurement that could show a hypothesis
is wrong, not just ones that could confirm it.

**scope** — The explicit boundary of what a measurement or claim covers
(which workload, which machine, which inputs) and, by implication, what it
does not cover.

**perturbation** — Any change introduced by the act of measuring or
experimenting itself, which can distort the very behavior being studied.
