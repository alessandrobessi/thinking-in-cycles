# Glossary

Terms are grouped by concept level (see `concept-graph.md`), alphabetical
within each group, then a **Supplementary vocabulary** section for teaching
terms Chapters 1–15 use that BLUEPRINT.md Section 11 doesn't formally list at
any level. Status as of this revision: Chapters 1–15 drafted.

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

## Levels 4–7

Not yet formally introduced by any drafted chapter. Term lists live in
`concept-graph.yaml`/`concept-graph.md`; definitions will be added as
Chapters 16–30 are drafted.

**Exception (informal use before formal treatment):** `on-CPU time` and
`off-CPU time` (formally Level 7, Chapter 29) are used informally
starting in Chapter 1 — see `concept-graph.md`'s "Known tensions"
section.

### on-CPU time
**Used informally in:** Chapter 1 · **Formal definition:** Chapter 29
Time a thread spends actually executing on a CPU.

### off-CPU time
**Used informally in:** Chapter 1 · **Formal definition:** Chapter 29
Time a thread spends not executing on a CPU for any reason — runnable and
waiting, blocked, or sleeping.

## Supplementary vocabulary

Plain teaching terms Chapters 1–15 introduce that Section 11 doesn't list
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
