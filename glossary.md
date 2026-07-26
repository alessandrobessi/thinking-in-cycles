# Glossary

Terms are grouped by concept level (see `concept-graph.md`), alphabetical
within each group, then a **Supplementary vocabulary** section for teaching
terms Chapters 1–30 use that the formal concept graph doesn't list at
any level. Status as of this revision: Chapters 1–30 drafted — the full
book is complete.

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

Ten of thirteen Level 1 terms are introduced by Chapters 21-22.
**process** and **thread** remain informal (used from Chapter 1 onward,
never given their own dedicated "New concepts" treatment); **user
time**, **system time**, and **wall time** remain as Chapter 1's guided
lab left them — compared operationally, without a full formal scheduler
model built around them specifically.

### blocked
**First introduced:** Chapter 21
A thread state in which a thread cannot run because it is waiting on an
event (I/O, a lock, a signal) rather than merely waiting for a CPU.

### context switch
**First introduced:** Chapter 22 (as "voluntary"/"involuntary context switch")
The act of a CPU stopping execution of one thread and beginning another,
saving and restoring the state needed to resume each.
**See also:** run queue, CPU migration

### CPU migration
**First introduced:** Chapter 22 (as "migration")
A thread resuming execution on a different logical CPU than the one it
last ran on.
**See also:** cache warmth

### process
An OS-managed unit of execution with its own address space, containing one
or more threads.

### run queue
**First introduced:** Chapter 21
The set of runnable threads waiting for a CPU on which to execute.

### runnable
**First introduced:** Chapter 21
A thread state in which a thread is ready to execute but is not currently
assigned a CPU.

### running
**First introduced:** Chapter 21
A thread state in which a thread is currently executing on a CPU.

### scheduler
**First introduced:** Chapter 21
The kernel subsystem that decides which runnable thread runs on which CPU
and for how long.

### sleeping
**First introduced:** Chapter 21
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

## Level 2 — CPU Work

All fifteen Level 2 terms are introduced by Chapters 6–10 (Part II is
complete as of this revision).

### back end
**First introduced:** Chapter 8
The part of a CPU pipeline containing the execution units that actually
carry out arithmetic, memory access, and control-flow operations.
**See also:** front end, pipeline, execution units

### branch prediction
**First introduced:** Chapter 9 (as "branch predictor")
Dedicated CPU hardware that guesses which way a conditional branch will
go, based on that branch's recent history, before the condition is
actually evaluated.
**See also:** speculation, misprediction

### CPI
**First introduced:** Chapter 7
Cycles per instruction: elapsed cycles divided by retired instruction
count — the reciprocal of IPC.
**See also:** IPC, cycle, instruction

### cycle
**First introduced:** Chapter 7
One tick of a CPU's clock — the basic unit of time a processor's
internal state advances by.
**See also:** CPU frequency, elapsed cycles, reference cycles

### dependency
**First introduced:** Chapter 8
A relationship where one instruction's input is another instruction's
output, forcing the dependent instruction to wait until the first one
finishes.
**See also:** stall, dependency chain

### front end
**First introduced:** Chapter 8
The part of a CPU pipeline responsible for fetching and decoding
instructions before handing them to the back end for execution.
**See also:** back end, pipeline, decode

### hardware performance event
**First introduced:** Chapter 10 (as "hardware event")
Something a CPU's PMU counts directly (cycles, instructions retired,
cache references), as distinct from a software event the kernel counts
on the CPU's behalf.
**See also:** PMU, software event

### instruction
**First introduced:** Chapter 6 (as "machine instruction")
A single, architecture-specific operation a CPU can execute — what a
compiler translates source code into.
**See also:** retired instruction, compiler

### IPC
**First introduced:** Chapter 7
Instructions per cycle: retired instruction count divided by elapsed
cycles — a measure of how much useful work a CPU packed into each clock
tick, for a specific, scoped measurement.
**See also:** CPI, cycle, instruction

### multiplexing
**First introduced:** Chapter 10
The kernel time-slicing a requested set of performance-counter events
across a CPU's limited physical counter registers, when more events are
requested than there are registers to count them simultaneously.
**See also:** PMU, scaling, event group

### PMU
**First introduced:** Chapter 10
Performance Monitoring Unit: dedicated CPU hardware that counts specific
architectural events without slowing down the code being measured.
**See also:** hardware performance event, multiplexing

### pipeline
**First introduced:** Chapter 8
The CPU's internal assembly-line-like structure for fetching, decoding,
and executing instructions, split into a front end and a back end.
**See also:** front end, back end, stall

### retired instruction
**First introduced:** Chapter 6
An instruction that has actually completed execution and had its effect
committed — the unit hardware performance counters count when they
report "instructions."
**See also:** instruction, IPC

### speculation
**First introduced:** Chapter 9 (as "speculative execution")
A CPU executing instructions down a predicted-but-not-yet-confirmed
branch path, before the branch's actual outcome is known.
**See also:** branch prediction, misprediction

### stall
**First introduced:** Chapter 8
A cycle in which some part of the pipeline has no useful work to do,
because the next instruction it would run is waiting on a dependency or
a memory access.
**See also:** dependency, memory wait

## Level 3 — Profiling

All thirteen Level 3 terms are introduced by Chapters 11–15 (Part III is
complete as of this revision).

### annotation
**First introduced:** Chapter 12
Mapping a function's sampled cost back onto its own source lines (or
disassembly), built on the debug information (source mapping) Chapter 6
first introduced.
**See also:** source mapping, self cost

### call graph
**First introduced:** Chapter 12
The tree of caller/callee relationships a profiler reconstructs from
sampled call stacks.
**See also:** call stack, inclusive cost

### call stack
**First introduced:** Chapter 12 (used informally from Chapter 11)
The chain of currently-active function calls at the moment a sample is
taken, from the outermost caller down to the innermost executing frame.
**See also:** call graph, stack unwinding

### counter
**First introduced:** Chapter 11 (used informally from Chapter 10)
A hardware- or software-tracked count of a specific event, read via a
tool like `perf stat` — used operationally throughout Chapter 10's `perf
stat` material before Chapter 11 formally contrasts counting with
sampling and tracing as three distinct observation models.

### debug information
**First introduced:** Chapter 13
Compiler-generated metadata (commonly DWARF format) mapping compiled
addresses back to source file and line, separate from and in addition
to the symbol table.
**See also:** symbol, DWARF, source mapping

### differential flame graph
**First introduced:** Chapter 15
A flame graph rendered from two folded-stack captures at once, coloring
each frame by whether its share of samples grew, shrank, or stayed
roughly the same relative to a baseline.
**See also:** flame graph, before/after profile

### flame graph
**First introduced:** Chapter 14
A visualization of aggregated, folded call stacks where each frame's
width is proportional to its share of total samples — not a timeline.
**See also:** folded stack, frame width

### frame pointer
**First introduced:** Chapter 13
A per-call-frame saved reference to the caller's frame, one common
mechanism a profiler's stack unwinder can follow to reconstruct a call
chain.
**See also:** stack unwinding

### sample frequency
**First introduced:** Chapter 12
How many samples a profiler takes per second — the inverse of the
period between samples.
**See also:** sample, period

### sampling
**First introduced:** Chapter 11
An observation model that periodically checks what's currently
executing, building a statistical picture of where execution tends to
be from many such snapshots.
**See also:** counting, tracing

### symbol
**First introduced:** Chapter 13 (as "symbol table")
A name (typically a function name) a compiled address can be mapped
back to, via the binary's symbol table — available independently of
debug information.
**See also:** debug information, stripped binary

### tracing
**First introduced:** Chapter 11
An observation model that records specific, individually meaningful
events as they happen, with timestamps and context, capable of catching
every occurrence of even a rare event.
**See also:** counting, sampling

### unwinding
**First introduced:** Chapter 13 (as "stack unwinding")
The process of reconstructing the chain of callers from a single
sampled snapshot, commonly via frame pointers or, on supporting
platforms, DWARF-based or last-branch-record-based alternatives.
**See also:** frame pointer, call stack

## Level 4 — Memory Behavior

All fourteen Level 4 terms are introduced by Chapters 16–20 (Part IV is
complete as of this revision).

### cache hit
**First introduced:** Chapter 17
An access satisfied by a cache level instead of going further out to a
slower one.

### cache line
**First introduced:** Chapter 16
The unit caches actually move data in (commonly 64 bytes) — touching
even one byte pulls in the whole line.
**See also:** L1/L2/LLC, locality

### cache miss
**First introduced:** Chapter 17 (as "compulsory miss" and "capacity
miss intuition")
An access not satisfied by a given cache level. A compulsory miss is
the unavoidable first-ever access to an address; misses beyond that
reflect the working set exceeding that level's capacity.
**See also:** working set, cache hit

### coherence
**First introduced:** Chapter 18
The protocol (commonly a MESI variant) that keeps multiple cores' cached
copies of the same cache line consistent, via ownership and
invalidation.
**See also:** false sharing, shared cache line

### false sharing
**First introduced:** Chapter 18
Cache-coherence traffic caused by unrelated variables that merely
happen to share a cache line, with no genuine logical contention
between the threads involved.
**See also:** coherence, true sharing

### L1/L2/LLC
**First introduced:** Chapter 16
Progressively larger, progressively slower layers of on-chip cache
sitting between registers and main memory; LLC is the last-level
(commonly shared) cache before DRAM.
**See also:** cache line, DRAM

### locality
**First introduced:** Chapter 16
The general principle that makes caching effective: spatial locality
(nearby addresses accessed close together in time) and temporal
locality (the same address accessed again soon).

### memory bandwidth
**First introduced:** Chapter 19 (as "sustained bandwidth"/"peak
bandwidth")
Bytes transferred per second between memory and the CPU; sustained
bandwidth is what's actually achieved under load, peak bandwidth is the
theoretical hardware maximum.
**See also:** bandwidth saturation, memory-level parallelism

### memory latency
**First introduced:** Chapter 16 (as "latency")
The time a single memory access takes to complete, growing sharply at
each layer further from the CPU.

### memory-level parallelism
**First introduced:** Chapter 19
How many memory requests a workload can keep outstanding at once —
near-zero for a dependent pointer chase, high for independent streaming
access.
**See also:** memory bandwidth, roofline intuition

### prefetching
**First introduced:** Chapter 17 (as "prefetcher")
Hardware speculatively loading data it predicts will be needed soon,
based on recently observed access patterns such as a constant stride.

### roofline intuition
**First introduced:** Chapter 19
The informal picture that a workload's achievable performance is
bounded by either compute throughput or bandwidth times arithmetic
intensity, whichever ceiling is lower for that workload.
**See also:** arithmetic intensity, memory bandwidth

### uncore
**First introduced:** Chapter 20
CPU-package circuitry outside the cores themselves — shared cache,
interconnect, and memory controller logic — with its own, separate
performance counters.
**See also:** memory controller

### working set
**First introduced:** Chapter 17
The specific data a program actually touches repeatedly during some
phase of execution, as opposed to all the memory it could touch.
**See also:** cache hit, cache miss, reuse

## Level 5 — Topology and Placement

All fifteen Level 5 terms are introduced by Chapters 23-25.

### cgroup
**First introduced:** Chapter 23 (as "cgroup CPU constraints")
A Linux kernel mechanism for grouping and constraining processes'
resource usage, commonly what cpusets and container CPU limits are
built on.

### core
**First introduced:** Chapter 23 (as "physical core")
A physical execution unit on a CPU package, potentially presenting as
more than one logical CPU under SMT.
**See also:** logical CPU, SMT

### CPU affinity
**First introduced:** Chapter 23
A restriction on which logical CPUs a thread is allowed to run on, set
via an affinity mask.
**See also:** affinity mask, cpuset

### cpuset
**First introduced:** Chapter 23
A named partition of CPUs (and memory nodes) that threads can be
restricted to.

### first-touch allocation
**First introduced:** Chapter 25
The default memory policy where a page isn't physically assigned to a
location until the first time something writes to it, at which point
it's placed on the node local to whichever CPU did the writing.
**See also:** local allocation policy, NUMA node

### isolation
**First introduced:** Chapter 23
Keeping specific CPUs mostly free of the kernel's own routine work
(via `isolcpus` and related configuration), reserved for
latency-sensitive threads.

### local memory
**First introduced:** Chapter 24
Memory attached to the same NUMA node as the CPU accessing it.
**See also:** remote memory, NUMA node

### logical CPU
**First introduced:** Chapter 23
One schedulable unit of execution from the OS's perspective; a single
physical core can present as more than one logical CPU under SMT.

### memory policy
**First introduced:** Chapter 25 (via "local allocation policy",
"interleave", "bind", "preferred node")
The rule governing where a thread's memory allocations are physically
placed relative to NUMA nodes.
**See also:** first-touch allocation, page migration

### NUMA distance
**First introduced:** Chapter 24 (as "distance")
A relative, often unitless number reported for how costly reaching a
given NUMA node's memory is from a given CPU.

### NUMA node
**First introduced:** Chapter 24
A group of CPUs and the memory directly attached to them, commonly
(though not always) one node per socket.
**See also:** local memory, remote memory, NUMA distance

### page migration
**First introduced:** Chapter 25
The kernel physically moving a page of memory to a different NUMA node,
typically to fix an access pattern that has become predominantly
remote.
**See also:** automatic NUMA balancing (supplementary, Chapter 25)

### remote memory
**First introduced:** Chapter 24
Memory attached to a different NUMA node than the CPU accessing it,
reachable only by crossing an interconnect.
**See also:** local memory, NUMA distance

### SMT
**First introduced:** Chapter 23 (as "SMT sibling")
Simultaneous multithreading: one physical core presenting as more than
one logical CPU to the scheduler, sharing the core's execution
resources.

### socket
**First introduced:** Chapter 23
A physical CPU package, potentially containing many cores.

## Level 6 — Dynamic Tracing

All seventeen Level 6 terms are introduced by Chapters 26-28.

### attachment point
**First introduced:** Chapter 27 (as "hook")
The specific point in the kernel (or, via uprobes, in user space) an
eBPF program attaches to and runs at — one of Chapter 26's tracepoints,
kprobes, kretprobes, or uprobes, among other kernel subsystems.
**See also:** eBPF program, tracepoint, kprobe

### BCC
**First introduced:** Chapter 28
A toolkit providing a Python/Lua front end and a library of maintained,
packaged eBPF tracing tools (`execsnoop`, `biolatency`, `runqlat`, and
others) for common investigations, avoiding the need to write a new
probe script for a question an existing tool already answers.
**See also:** bpftrace

### bpftrace
**First introduced:** Chapter 28
A small scripting language that compiles down to verified eBPF
bytecode at invocation time, built for short, inspectable one-liner
questions rather than a full libbpf application.
**See also:** BCC, probe specification (supplementary, Chapter 28)

### BTF
**First introduced:** Chapter 27
BPF Type Format: embedded type information describing kernel and user
data structures precisely, the foundation CO-RE builds on.
**See also:** CO-RE

### CO-RE
**First introduced:** Chapter 27
"Compile Once, Run Everywhere" — using a target kernel's own BTF
information to let one compiled eBPF program adapt to small differences
in struct layout across kernel versions, without requiring a fresh
compile per target.
**See also:** BTF

### eBPF program
**First introduced:** Chapter 27
A small, restricted piece of code, loaded into the kernel and run at a
specific attachment point, that the verifier has proven terminates and
only accesses memory it can prove is safe before it is ever allowed to
execute.
**See also:** verifier, attachment point, helper

### event
**First introduced:** Chapter 26
A specific, nameable thing that happens during execution — a function
being entered or returning, a system call being made — that dynamic
tracing can attach measurement to directly.
**See also:** tracepoint, kprobe

### helper
**First introduced:** Chapter 27
One of a fixed, kernel-provided set of functions an eBPF program may
call, for the specific operations a verified, sandboxed program is
still permitted to perform.
**See also:** eBPF program, verifier

### histogram
**First introduced:** Chapter 28
An aggregation that buckets a value's distribution — such as a
duration — rather than only summing or counting it.
**See also:** aggregation (supplementary, Chapter 28)

### kprobe
**First introduced:** Chapter 26
A dynamic probe attached to the entry of almost any kernel function by
name, even one never designed to be probed — flexible, but fragile
across kernel-version changes that rename, inline, or restructure the
targeted function.
**See also:** kretprobe, uprobe, tracepoint

### kretprobe
**First introduced:** Chapter 26
The matching probe type for a kernel function's *return*, which paired
with a kprobe's entry timestamp is what makes a function call's
duration measurable at all.
**See also:** kprobe

### map
**First introduced:** Chapter 27
A kernel-resident data structure an eBPF program and user space can
both access, used to aggregate state (counts, sums, histograms) across
many event firings entirely inside the kernel.
**See also:** per-CPU map (supplementary, Chapter 27), ring buffer

### ring buffer
**First introduced:** Chapter 27
A map variant purpose-built for streaming individual events out to
user space efficiently, rather than only aggregating them in place.
**See also:** map

### tracepoint
**First introduced:** Chapter 26
An event location the kernel's own developers deliberately built in and
documented, with a stable name and defined arguments — the kernel
equivalent of a designed API, more stable across versions than a kprobe.
**See also:** kprobe, USDT

### uprobe
**First introduced:** Chapter 26
The user-space equivalent of a kprobe: a dynamic probe attached to
almost any function in a running binary or shared library.
**See also:** kprobe, USDT

### USDT
**First introduced:** Chapter 26
User-space statically-defined tracing: a deliberately placed, named,
documented probe point an application's own authors built in on
purpose — to uprobes roughly what a tracepoint is to a kprobe.
**See also:** uprobe, tracepoint

### verifier
**First introduced:** Chapter 27
The kernel component that statically analyzes an eBPF program before
it loads, refusing anything it cannot prove terminates and only
accesses memory it can prove is safe.
**See also:** eBPF program

## Level 7 — Whole-System Diagnosis

All thirteen Level 7 terms are introduced. Several were used
informally/operationally well before their formal chapter — see
`concept-graph.md`'s "Resolved tensions" section.

### bottleneck shift
**Used informally in:** Chapter 5 · **Formal definition:** Chapter 30
A confirmed bottleneck's removal revealing the next-most-limiting
resource underneath it, rather than removing the workload's ceiling
entirely — success, not an incomplete fix.
**See also:** regression

### causal claim
**Used informally in:** Chapter 5 · **Formal definition:** Chapter 30
A specific, falsifiable assertion that one particular change produced
one particular measured effect, supported by a measurement chosen to
distinguish that explanation from its plausible alternatives.

### futex
**First introduced:** Chapter 29 (as "futex wait")
The Linux mechanism most user-space lock implementations (including
`pthread_mutex`) use to put a thread to sleep efficiently while
contended, rather than spinning and burning CPU while waiting.
**See also:** lock contention

### interference
**Used informally in:** Chapter 22 · **Formal definition:** Chapter 30
Unrelated work degrading a workload's performance by sharing a
contended resource with it — Chapter 22's "noisy neighbor" lab is a
specific, cross-process instance of this same concept.

### I/O latency
**First introduced:** Chapter 29 (via "block I/O")
The round-trip time for a request to a storage or network device; the
issuing thread is typically off-CPU for the entire wait.
**See also:** off-CPU time, queueing delay

### lock contention
**First introduced:** Chapter 29
Multiple threads wanting the same lock at overlapping times, with at
least one necessarily spending time off-CPU as a direct result.
**See also:** futex, off-CPU time

### off-CPU time
**Used informally in:** Chapter 1, Chapter 21 · **Formal definition:** Chapter 29
Time a thread spends not executing on a CPU for any reason — runnable and
waiting, blocked, or sleeping.
**See also:** on-CPU time, blocked stack (supplementary, Chapter 29)

### on-CPU time
**Used informally in:** Chapter 1, Chapter 21 · **Formal definition:** Chapter 29
Time a thread spends actually executing on a CPU.
**See also:** off-CPU time

### queueing delay
**First introduced:** Chapter 29
Time spent waiting for a resource — a lock, a run queue slot, an I/O
device — that is busy or contended, distinct from the time actually
using that resource once available.
**See also:** lock contention, I/O latency

### regression
**First introduced:** Chapter 30
A measured, confirmed worsening from a change, checked for with the
same before/after discipline (Chapter 15) that confirms an improvement.
**See also:** bottleneck shift

### scalability
**First introduced:** Chapter 30
How a workload's throughput or latency changes as some resource (most
often thread or worker count) increases.

### tail latency
**First introduced:** Chapter 3
The latency of the slowest fraction of requests (commonly p95, p99, or
p999), which an average or median can hide entirely. Chapter 29's
off-CPU model extends this concept to whole-system waiting without a
second formal introduction.
**See also:** latency, throughput

### wake-up latency
**First introduced:** Chapter 29
The gap between a thread becoming logically able to run again (its
dependency resolved, its I/O complete) and actually resuming
execution — Chapter 21's run-queue waiting, named from the waiting
thread's own perspective.
**See also:** run queue, off-CPU time

## Supplementary vocabulary

Plain teaching terms Chapters 1–30 introduce that Section 11 doesn't list
at any formal level. Tracked here so they're not silently invented; not
part of `concept-graph.yaml`'s level structure.

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

### Chapter 6

**compiler** — A program that translates source code into machine
instructions for a specific target architecture.

**machine instruction** — A single, architecture-specific operation a CPU
executes (load, add, branch, etc.), as opposed to a line of source code.

**micro-operation intuition** — The informal awareness that CPUs often
break machine instructions down further internally before executing them;
covered by name only in Chapter 6, mechanics deferred to Chapter 8.

**optimization** — Any compiler transformation that makes the emitted
instructions do the same job with less work.

**vectorization** — A compiler optimization that uses instructions
operating on multiple data elements at once instead of one at a time.

**inlining** — Replacing a function call with the callee's body directly,
removing call overhead and enabling further optimization across the
former call boundary.

**dead-code elimination** — Removing computations whose results provably
cannot affect a program's observable output.

### Chapter 7

**instruction count** — How many machine instructions actually retired
while running some piece of code.

**elapsed cycles** — Actual CPU clock ticks counted during a measurement.

**reference cycles** — Clock ticks counted at a fixed reference rate,
independent of the CPU's actual (dynamically scaled) frequency.

**CPU frequency** — The CPU's actual clock speed, which can change
dynamically due to thermal state and power management.

### Chapter 8

**decode** — The pipeline stage that translates fetched instructions into
a form the rest of the CPU can execute.

**execution units** — The back end components that actually carry out
arithmetic, memory access, and control-flow operations.

**out-of-order execution** — A CPU running instructions in an order other
than program order, when doing so lets it use otherwise-idle execution
units on instructions whose inputs are already ready.

**memory wait** — A stall caused by an instruction's input depending on a
cache or memory access that hasn't returned yet.

**issue width intuition** — The informal ceiling on how many instructions
a CPU can start per cycle, regardless of how much independent work is
available.

### Chapter 9

**branch** — Any instruction whose next instruction depends on a runtime
condition (an `if`, a loop check, a `switch`).

**branch predictor** — Dedicated CPU hardware that guesses which way a
branch will go before its condition is evaluated.

**misprediction** — A branch predictor's guess turning out wrong, forcing
the CPU to discard speculatively executed work and restart down the
correct path.

**speculative execution** — A CPU executing instructions down a
predicted-but-unconfirmed path before the branch's actual outcome is known.

**dependency chain** — A sequence of instructions where each one's input
is the previous one's output, forcing strictly sequential execution.

**branchless trade-off** — The choice between a branch (cheap if
well-predicted, expensive if mispredicted) and unconditional code that
always does the same fixed amount of work regardless of outcome.

### Chapter 10

**hardware event** — Something a CPU's PMU counts directly, such as
cycles or retired instructions.

**software event** — Something the kernel counts on the CPU's behalf
(context switches, page faults), reported through the same `perf stat`
interface as hardware events despite a different counting mechanism.

**event group** — A set of performance-counter events `perf stat` is
asked to count together, guaranteed to be measured over the same interval.

**scaling** — `perf stat` extrapolating a full-run estimate for a
multiplexed event from the fraction of the run it was actually counted
during.

**per-thread versus system-wide measurement** — The distinction between
measuring one process/thread's counters and measuring an entire
machine's activity (`perf stat -a`) over a fixed duration.

**privilege restrictions** — The permission model (root,
`perf_event_paranoid`, `CAP_PERFMON`) governing who can read hardware
performance counters on a given Linux system.

### Chapter 11

**counting** — An observation model answering "how many/how much,"
built from a running tally with no information about when or where each
event happened.

### Chapter 12

**sample** — One snapshot of what's currently executing, taken by a
profiler.

**period** — How much time (or how many events) elapse between samples.

**overhead percentage** — A sample count expressed as a share of total
samples taken — a statistical estimate of time share, not a direct time
measurement.

**inclusive versus self cost** — Inclusive cost is the share of samples
where a function appears anywhere in the active call stack, including
everything it called; self cost is the share where it's the innermost,
currently-executing frame.

**source mapping** — The underlying debug-information mechanism
connecting compiled addresses back to source file and line, which makes
annotation possible.

### Chapter 13

**stripped binary** — A binary that has had some or all of its symbol
table and/or debug information deliberately removed.

**DWARF** — The debug information format most Linux and macOS compilers
emit, mapping compiled addresses back to source file and line.

**last branch records** — A hardware feature on some CPUs recording
recent branch history, usable as an alternative stack-unwinding source
independent of frame pointers.

**JIT symbols** — Symbol information for code generated at runtime by a
JIT compiler, which a static binary's symbol table cannot describe on
its own.

**kernel symbols** — Symbol information for addresses inside kernel
code, subject to its own separate permission and availability rules
(e.g. `kptr_restrict`).

### Chapter 14

**folded stack** — One line of text representing one unique call path
and how many samples landed there (`frame1;frame2;frame3 count`), the
intermediate format between raw captured samples and a rendered flame
graph.

**stack aggregation** — Merging every sample that shares an identical
call path into one folded-stack line.

**frame width** — The only flame graph dimension that encodes
magnitude, proportional to a frame's share of total samples.

**ancestry** — A frame's chain of parents going down to the root.

**plateau** — A wide flame graph frame with little rising above most of
its width, suggesting real self cost.

**tower** — A full vertical stack of frames from base to top in a flame
graph, representing one call path's full depth.

**off-CPU flame graph** — A flame graph built from time spent blocked
or waiting rather than executing, requiring a different capture method
than a CPU flame graph since on-CPU sampling cannot see off-CPU time.

### Chapter 15

**before/after profile** — A pair of profiles capturing the same
workload's shape both before and after exactly one change.

**normalized workload** — A workload held constant, in an explicitly
stated way (same input, same duration, or same completed work), so a
before/after comparison is actually comparable.

**`perf diff`** — The Linux `perf` subcommand for an aggregate,
non-visual before/after profile comparison.

**regression** — A change that made some metric worse, caught by the
same before/after discipline used to catch improvements.

**total-work normalization** — Making explicit whether a before/after
comparison is normalized by time, by completed operations, or by
another unit — since equal wall-clock duration does not mean equal
completed work when one version is faster.

### Chapter 16

**register** — The fastest storage a CPU has, directly wired into its
execution units, effectively free to access.

**DRAM** — Main memory itself: much larger than any cache level, and
much slower to reach.

### Chapter 17

**stride** — The fixed distance advanced between successive accesses in
a regular access pattern.

**reuse** — Accessing the same data again while it's still cached.

**TLB (as a forward pointer)** — A small cache of virtual-to-physical
address translations, itself subject to the same hit/miss dynamics as a
data cache and itself capacity-limited.

### Chapter 18

**shared cache line** — A cache line more than one core has a copy of.

**ownership** — A cache line's state under the coherence protocol,
determining which core(s) may read or write it without first
coordinating with others.

**invalidation** — A core writing to a line it doesn't exclusively own
forcing every other core's copy of that line to be discarded.

**coherence traffic** — Real bus/interconnect activity generated by
ownership transfers and invalidations, invisible to source code.

**true sharing** — Coherence traffic from genuinely shared data that
multiple threads actually read and write.

**HITM intuition** — The informal sense of a hardware event where a
read or write "hits" a cache line another core has modified, the
underlying signal cache-to-cache-transfer tools detect directly.

### Chapter 19

**bytes transferred** — The total data moved between memory and the CPU
over some measurement.

**sustained bandwidth** — Bytes transferred per second under real,
continued load.

**peak bandwidth** — The theoretical maximum a memory system could ever
deliver; a hardware specification, essentially never achieved in
practice.

**arithmetic intensity** — The ratio of useful computation to bytes
transferred (operations per byte), separating compute-bound workloads
from bandwidth-bound ones.

**bandwidth saturation** — The point past which additional concurrent
demand cannot increase delivered throughput, because the memory
channels are already moving data near their sustainable rate.

### Chapter 20

**load/store sampling** — Sampling individual memory-access instructions
specifically (as `perf mem` does), rather than instruction-pointer
sampling generally, tagging each with a data source.

**data source** — Which cache level or memory location satisfied a
sampled memory access.

**local/remote classification** — On NUMA systems, whether a sampled
access was satisfied by memory local to the accessing core's node or
remote to it.

**cache-to-cache transfer** — A cache line moving directly from one
core's cache to another's rather than from a shared cache or memory,
the event `perf c2c` is built to detect.

**memory controller** — The uncore logic responsible for actually
issuing reads and writes to DRAM on behalf of the cores.

### Chapter 21

**time slice intuition** — The informal sense that a running thread
gets a bounded slice of time before the scheduler reconsiders who
should run next, especially when others are waiting.

**wake-up** — A sleeping thread becoming runnable again, in response to
whatever it was waiting for becoming available.

**load balancing** — The scheduler's ongoing effort to keep runnable
work spread reasonably evenly across available CPUs.

**scheduling class** — The policy category a thread belongs to (e.g.
the default time-shared class, or a real-time class), affecting how
scheduling mechanics apply to it.

### Chapter 22

**cache warmth** — The accumulated benefit of a thread's data already
sitting in a specific core's cache, discarded (at least partly) by a
context switch or migration.

**interrupt** — A signal that diverts a CPU to handle an event
immediately, which can concentrate real cost on whichever CPU handles
it most.

**steal time** — In a virtualized environment, CPU time a guest was
ready to use but couldn't, because the hypervisor gave it to something
else.

**noisy neighbor** — Unrelated, co-located work competing for the same
shared CPUs (or other resources), degrading a workload's performance
with no code-level relationship to it.

**CPU pressure** — The general sense of demand for CPU time exceeding
what's readily available, whether from a workload's own threads or
from unrelated neighbors.

### Chapter 23

**affinity mask** — A specification of exactly which logical CPUs a
thread is allowed to run on.

**`taskset`** — The Linux command-line tool for setting a process's CPU
affinity.

**`sched_setaffinity`** — The Linux system call underlying `taskset`
and similar tools.

### Chapter 24

**interconnect** — The physical link between NUMA nodes (commonly
sockets), slower and lower-bandwidth than a node's own direct memory
connection.

**node topology** — The overall map of how many NUMA nodes exist and
how they're connected.

**memory-only node** — A NUMA node with no CPUs attached, whose memory
is reachable only remotely.

### Chapter 25

**local allocation policy** — A memory policy explicitly requesting
first-touch behavior.

**interleave** — A memory policy that spreads a buffer's pages
round-robin across multiple NUMA nodes deliberately.

**bind** — A memory policy that forces allocation onto one specific
NUMA node regardless of which CPU touches it first.

**preferred node** — A softer memory-placement hint than bind: try this
node, fall back if it's full.

**automatic NUMA balancing** — The kernel's background effort to notice
a page being accessed predominantly from a remote node and consider
migrating it closer.

**NUMA hit/miss statistics** — Counters reporting how often memory
accesses were satisfied locally versus remotely.

### Chapter 26

**function entry/return** — The two moments a kprobe/kretprobe pair (or
a uprobe pair) can attach to for one function, together making a call's
duration measurable.

**argument capture** — Reading the specific values present at the
moment a probe fires (a function's arguments, a return value, a
timestamp), turning "this event happened" into a usable measurement.

**event rate** — How often a given probe actually fires per unit time
in practice; the single biggest lever on how much overhead attaching
that probe will cost.

### Chapter 27

**per-CPU map** — A map variant keeping a separate copy per CPU,
avoiding the cross-core coherence cost a single shared counter updated
from every CPU would incur.

**user-space loader** — The ordinary program (compiled from `bpftrace`,
BCC, or a hand-written libbpf application) that presents eBPF bytecode
to the kernel, reads back map contents, and manages a program's
lifecycle.

### Chapter 28

**probe specification** — The part of a `bpftrace`/BCC script naming
the hook a piece of logic attaches to.

**predicate** — An optional filter in a `bpftrace`/BCC script: run the
attached logic only when some condition holds.

**action** — The logic that runs when a probe fires and its predicate
(if any) passes, most commonly updating an aggregation.

**aggregation** — `bpftrace`'s built-in map-like construct handling the
key/value bookkeeping a map provides underneath a short script.

**stack aggregation** — An aggregation keyed by call stack instead of a
process name or ID, turning "which code paths do this most" into the
same count-grouped-by-key shape as any other aggregation.

**interval output** — Printing an aggregation's current state on a
fixed schedule, rather than only once at the end.

### Chapter 29

**blocked stack** — The call stack captured at the moment a thread
stopped running, showing where in the code it stopped.

**sleep** — A thread intentionally yielding the CPU for a bounded time,
an off-CPU state with no external blocker.

**block I/O** — A request to a block storage device (a disk read, for
instance); the issuing thread is typically off-CPU for the entire
round trip.

**off-CPU flame graph** — Flame graph's population-of-stacks
visualization (Chapter 14) applied to blocked stacks and their off-CPU
durations instead of on-CPU sampled time. First informally named in
Chapter 14's supplementary vocabulary; formally defined here.

### Chapter 30

No new supplementary vocabulary — this chapter synthesizes Chapters
1-29 and formally completes five Level 7 terms already listed above
(bottleneck shift, causal claim, interference, regression, scalability)
rather than introducing new teaching vocabulary of its own.
