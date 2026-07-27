# Cycles, Instructions, IPC, and CPI

**Part:** Part II — What the CPU Is Doing
**Concept level:** 2
**Prerequisites:** compiler, machine instruction, retired instruction (Chapter 6)
**New concepts:** cycle, instruction count, IPC, CPI, elapsed cycles, reference cycles, CPU frequency

## Opening Question

What do cycles and instructions tell us?

## Incident or Real-World Story

Two engineers each optimize a different part of the same data-processing
pipeline. The first proudly reports that their rewritten function now
executes 40% fewer instructions per record. The second reports no change
in instruction count at all, but a 25% drop in wall-clock time for the
same workload. In a review meeting, the first result sounds more
impressive — a 40% cut sounds like a bigger win than "no change." When
the two changes are actually benchmarked together in the full pipeline,
the second one accounts for nearly all of the pipeline's measured
speedup. The first one, despite executing far fewer instructions, barely
moves the needle.

What the first engineer's number didn't capture is *how expensive* each
of those instructions was. Their rewrite cut a redundant loop, which
genuinely reduced instruction count — but the remaining instructions were
already fast, well-pipelined, and a small fraction of the function's
total time to begin with. The second engineer's change didn't touch
instruction count at all; it restructured a dependency chain so the CPU
could keep more execution units busy at once, cutting how many cycles
each instruction took on average. Instruction count and elapsed time are
related, but neither one alone tells the whole story, and reporting
either in isolation invites exactly the wrong conclusion this story shows.

## Predict Before Measuring

Before reading further: if two versions of a function execute the exact
same sequence of arithmetic instructions, in the same order, but one
version gives the CPU multiple independent pieces of that arithmetic to
work on at once while the other forces every operation to wait for the
one before it — do you expect their elapsed times to be close to equal,
or meaningfully different? If different, by roughly how much — tens of
percent, or multiples? Hold that answer; you measured something adjacent
to this exact scenario using `--chains` in Chapter 8's preview material
below, and you'll measure it directly in this chapter's lab.

## Worked Example

Picture two implementations of the same numeric task:

- **Implementation A** retires *fewer* total instructions to finish the
  task, but each instruction spends more time waiting — for a value that
  isn't ready yet, for a slow memory access, for a preceding instruction
  in a dependency chain.
- **Implementation B** retires *more* total instructions, but the CPU can
  keep many of them moving through execution at once, so each one costs
  less average time.

Implementation B can easily finish sooner than Implementation A despite
doing more nominal "work" by instruction count — exactly the shape of
this chapter's opening story. Instruction count describes *how much* the
CPU was asked to do; it says nothing on its own about *how efficiently*
the CPU was able to do it.

## Core Intuition

A **cycle** is one tick of the CPU's clock — the basic unit of time a
processor's internal state advances by. **Instruction count** is how many
machine instructions actually retired while running some piece of code.
**IPC** (instructions per cycle) is instruction count divided by elapsed
cycles — a measure of how much useful work the CPU packed into each tick,
on average, for that specific run. **CPI** (cycles per instruction) is
its reciprocal — how many cycles each instruction cost on average. Both
are ratios over a specific, scoped measurement; neither is a fixed
property of a CPU or a program in the abstract. **Elapsed cycles** counts
actual clock ticks during a measurement; **reference cycles** counts
ticks at a fixed reference rate, which matters because modern CPUs change
their actual clock frequency (**CPU frequency**) dynamically — comparing
elapsed-cycle counts across two runs at different frequencies compares
two different things unless that's accounted for.

## Technical Explanation

IPC and CPI are two views of the same ratio, and both depend entirely on
*what the CPU was asked to do* and *what the microarchitecture makes
possible*, not on some universal notion of processor speed. A workload
with many independent, easily-pipelined operations can sustain a high
IPC; a workload with long dependency chains, frequent branches, or
frequent memory stalls will show a lower IPC on the *same* CPU, running
the *same* clock frequency — the difference is in the work's shape, not
the hardware. This is why "our code has an IPC of 2.1" is not, by itself,
a meaningful performance claim: 2.1 compared to what workload, on what
CPU, measured over what scope?

Frequency adds a second wrinkle worth naming now and returning to later:
if a CPU's clock speed changes between two runs — due to thermal
throttling, power management, or turbo boost — then elapsed cycles and
elapsed wall time stop being simply proportional to each other, and a
cycle-based comparison across the two runs needs to account for that or
it will misattribute a frequency change to a difference in the code
itself. Chapter 4's thermal-state and frequency-scaling cautions apply
here in a very literal, mechanical way.

## Tool View

- What is measured: task-clock, elapsed cycles, retired instructions, and
  elapsed wall time for a specific, scoped run — from which IPC/CPI are
  computed.
- What is not measured: *why* IPC is what it is — a single IPC number
  doesn't distinguish a branch-misprediction-limited workload from a
  memory-stall-limited one from a genuinely well-pipelined one; that
  requires the more targeted counters Chapter 8 and Part IV introduce.
- Required permissions (Linux, via `perf stat`): reading hardware
  performance counters typically requires either root or a permissive
  `perf_event_paranoid` setting — see Chapter 10 for the full permissions
  discussion.
- Likely overhead: `perf stat`'s counting overhead is normally small
  relative to the workload being measured, but is not zero; treat it the
  same as any other measurement tool from Chapter 4's hygiene checklist.
- Portability: `perf stat` is Linux-only. On a Linux machine with
  hardware counter access, the commands below are the standard way to
  get IPC directly:

  ```bash
  perf stat -e task-clock,cycles,instructions -- \
    ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=1 --op=int --chains=1
  perf stat -e task-clock,cycles,instructions -- \
    ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=1 --op=int --chains=8
  ```

  **These two commands are documented, not tested** — the reference
  machine for this book is macOS, where `perf` does not exist. Their
  syntax follows `perf`'s stable, documented event names; if you have
  Linux with counter access, run them and compare the reported `insn per
  cycle` line between the two.
- Common failure mode: comparing IPC between two runs of genuinely
  different workloads (different input sizes, different operations) and
  treating the difference as if it were caused by a code change rather
  than by measuring two different things — the same "compatible scope"
  caution Chapter 2 raised about metrics generally, applied here to
  counters specifically.

## Guided Lab

**Portability:** portable (primary lab below); the `perf stat` commands
in Tool View are **hardware-dependent** / Linux-only for readers who want
to see IPC numbers directly.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch7_ipc_intuition.sh
```

This runs the exact same per-iteration instruction mix (`--op=int`, 16
unrolled arithmetic operations per iteration either way) once as a single
dependency chain (`--chains=1`) and once as eight independent chains
(`--chains=8`), so any throughput difference between the two comes from
how efficiently the CPU could pack those *same* instructions into time,
not from doing different amounts of work.

**Expected qualitative result:** `--chains=8` should show substantially
higher throughput than `--chains=1`, despite executing the identical
instruction sequence per iteration. One example run on the reference
machine for this book (Apple M4, macOS, arm64) showed:

```text
chains=1: throughput_ops_per_s ≈ 724,000,000
chains=8: throughput_ops_per_s ≈ 2,227,000,000
```

Roughly a 3.1x difference, from an instruction-mix change of exactly
zero.

**Interpretation:** this is what a large IPC difference looks like from
the outside, without reading the counter directly — the same instructions
retiring much faster, on average, once the CPU has independent work to
interleave. A reader with `perf stat` access on Linux can confirm this
directly: run the two Tool View commands above and compare the reported
`instructions` and `cycles` counts (`insn per cycle` in `perf stat`'s own
summary line) between the two — expect a noticeably higher IPC for
`--chains=8`, in rough proportion to the throughput difference measured
here.

**Fallback path:** already this chapter's primary path — the portable
`cyclelab`-only comparison stands on its own without `perf`. If `perf` is
available, treat it as confirmation, not a required step.

**Cleanup:** none.

## Common Misconceptions

### *"Higher IPC always means better performance." (M04)*

**Why it's wrong:** IPC describes pipeline utilization for a specific
workload on a specific CPU; elapsed time and completed work remain the
primary measure of whether something is actually faster, and a workload
can have a high IPC while still being far slower than a lower-IPC
alternative that simply does less total work.

**Correct intuition:** Compare elapsed time and completed work directly,
alongside IPC, rather than ranking two implementations by IPC alone —
Chapter 2's "faster for what workload" question applies to counters
exactly as much as to benchmarks.

**Analogy:** A factory line that never stops moving looks maximally
efficient by one measure, but if it's stamping out parts nobody ordered,
its high "utilization" says nothing about whether the factory is
actually productive.

### *"Fewer instructions always means faster code." (M03, revisited)*

**Why it's wrong:** This chapter's opening story is a direct instance of
M03 (first introduced in Chapter 6): fewer retired instructions did not
translate into a meaningful speedup, because the instructions removed
weren't the ones costing the most cycles.

**Correct intuition:** IPC and CPI are the vocabulary this chapter adds
for explaining *why* — cost per instruction varies enormously depending
on dependencies, stalls, and pipeline behavior, subjects Chapter 8 opens
directly.

**Analogy:** Cutting three quick errands from a ten-item to-do list
saves less time than cutting the one item that involves waiting in line
at the DMV — counting items removed tells you nothing about which items
were actually expensive.

## Practical Implications

Before citing an instruction-count or IPC number as evidence a change
helped, pair it with elapsed time and completed work under the same
controlled conditions from Chapter 4. Neither instruction count nor IPC,
alone, is a performance verdict — both are diagnostic inputs that only
mean something once elapsed time and workload scope are also on the
table.

## Key Takeaway

**Cycles and instructions describe how work executed, but neither is a
performance verdict without elapsed time and workload context.**

## What to Remember

- A cycle is one clock tick; instruction count is how many instructions
  actually retired; IPC and CPI are two views of the same ratio between
  them.
- IPC and CPI are properties of a specific workload running on a specific
  microarchitecture, not fixed properties of either alone.
- A high IPC is not automatically good, and a low IPC is not
  automatically bad — both depend on what the workload's instructions
  actually needed to do.
- CPU frequency changes complicate cycle-based comparisons across runs;
  Chapter 4's thermal/frequency-scaling cautions apply directly here.
- Comparing counters across two different workloads or input sizes
  compares two different things, not a before/after of the same thing.
- The same instruction sequence can retire far faster with independent
  work available to interleave than without it — measurable even without
  reading a hardware counter directly, as this chapter's lab shows.
- `perf stat` is the standard Linux tool for reading these counters
  directly; Chapter 10 covers it in depth.

## Further Reading

- Linux perf manual pages: <https://man7.org/linux/man-pages/man1/perf.1.html>
- Linux perf documentation: <https://docs.kernel.org/admin-guide/perf/index.html>

## The Next Obvious Question

Why can a CPU spend cycles without retiring useful work?
