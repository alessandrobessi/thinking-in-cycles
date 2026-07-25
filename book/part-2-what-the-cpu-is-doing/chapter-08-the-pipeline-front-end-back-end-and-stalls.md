# Chapter 8 — The Pipeline: Front End, Back End, and Stalls

**Part:** Part II — What the CPU Is Doing
**Concept level:** 2
**Prerequisites:** cycle, instruction count, IPC, CPI (Chapter 7)
**New concepts:** front end, decode, execution units, back end, out-of-order execution, stall, dependency, memory wait, issue width intuition

## Opening Question

Why can a CPU spend cycles without retiring useful work?

## Incident or Real-World Story

A performance engineer is handed two functions that, by every static
measure available in Chapter 6 and 7's terms, look nearly identical: same
instruction count, same mix of arithmetic operations, compiled by the
same compiler at the same optimization level. One runs noticeably slower
than the other. Diffing the source doesn't reveal anything — until the
engineer notices that the slow version accumulates into a single running
total across an entire loop, while the fast version accumulates into
four separate running totals and only sums them at the end.

Both versions do the same arithmetic, in the same order source-wise, at
the same instruction count. The difference is that the slow version's
every operation must wait for the previous one to finish before it can
even begin, because each one needs the last one's result — a strict
chain, one link at a time. The fast version's four separate totals don't
depend on each other at all, so the CPU can work on more than one of
them at once. Nothing about "how much work" changed. What changed is how
much of that work the CPU was *allowed* to do simultaneously.

## Predict Before Measuring

Before reading further, revisit your Chapter 7 lab result (chains=1 vs.
chains=8) with a new question in mind: what do you think happens if you
keep increasing the chain count well past 8 — to 12, to 16? Does
throughput keep climbing in proportion, or does it eventually stop
climbing even though there's still more independent work available to
give the CPU? Write down a guess before running this chapter's lab.

## Worked Example

The canonical mental picture, useful specifically because it's easy to
outgrow correctly: a CPU pipeline is like a multi-stage assembly line
whose later stations can work *out of order* when the materials they
need are ready, but which still stalls if the right materials — or the
next instruction to work on — aren't available yet. Unlike a literal
assembly line, a modern CPU's execution stations aren't limited to
processing items in the order they arrived; **out-of-order execution**
lets the CPU look ahead at nearby instructions and start any of them
whose inputs are already available, skipping over ones that are still
waiting. That's precisely why the four-independent-accumulators version
in this chapter's story ran faster: with no instruction waiting on
another, the CPU had far more freedom to keep multiple execution
stations busy at once. The single-accumulator version gave it almost no
such freedom — every instruction had exactly one thing it could possibly
be waiting on, and it always was.

## Core Intuition

A CPU pipeline has (at least) two broad halves: a **front end**
responsible for fetching and **decode**-ing instructions into a form the
rest of the machine can execute, and a **back end** containing the
actual **execution units** that carry out arithmetic, memory access, and
control-flow operations. A **stall** is a cycle in which some part of the
pipeline has no useful work to do — not because the CPU is broken, but
because the next instruction it would want to run is waiting on
something: a **dependency** (an input that isn't ready yet, because a
prior instruction hasn't finished producing it), or a **memory wait** (an
input that has to come from a cache or memory access that hasn't
returned yet — Part IV's subject in full). **Issue width intuition** is
the informal ceiling this chapter's lab makes visible: a CPU can only
start so many instructions per cycle, no matter how much independent
work is available, so throughput gains from adding more independent work
taper off once that ceiling is reached.

## Technical Explanation

Chapter 7 framed stalls in terms of what they do to IPC; this chapter is
about what actually causes them. A dependency chain — one instruction's
result feeding directly into the next — forces the back end to execute
those instructions in strict order regardless of how many *other*,
independent execution units sit idle nearby, because there is nothing
else ready to give them. This is exactly what limited Chapter 7's
`--chains=1` run: one chain, one instruction at a time able to proceed,
no matter how wide the underlying pipeline actually was.

Adding independent chains gives the out-of-order machinery more choices
about what to run next when one chain is waiting on its own prior step —
which is why throughput rose sharply from `--chains=1` toward the middle
of this chapter's lab range. But that benefit has a ceiling: a real CPU
has a finite issue width (how many instructions it can dispatch per
cycle), a finite number of execution units, and finite internal buffering
for tracking in-flight instructions. Past some number of independent
chains, adding more doesn't create more usable parallelism — it just
creates more work waiting for the same finite resources, and throughput
flattens. That flattening, not the initial rise, is this chapter's
central evidence: **a stall is specifically a failure to supply
independent work**, and once independent work is no longer the limiting
factor, adding more of it stops helping.

## Tool View

- What is measured: throughput as a function of available independent
  work (`--chains`), as an indirect but real signal of pipeline behavior.
- What is not measured: which *specific* resource (issue width,
  execution unit count, reorder buffer size) is the limiting factor at
  the plateau — that requires microarchitecture-specific counters this
  chapter deliberately doesn't require, in keeping with Section 7.3's
  "one new mental model at a time."
- Required permissions: none for this chapter's lab.
- Likely overhead: negligible.
- Portability: works anywhere `cyclelab` runs.
- Tool policy note: a fuller, counter-based version of this investigation
  exists under the name "top-down microarchitecture analysis," which
  attributes stalled cycles to specific pipeline stages using hardware
  events. It requires vendor- and generation-specific event definitions
  and is deliberately out of scope here — treat it as the natural next
  step for a reader with Linux `perf` access and a specific CPU's
  documentation in hand, not as something this chapter's portable lab
  needs.
- Common failure mode: assuming a plateau means the CPU is "maxed out"
  in some general sense, rather than specifically out of independent
  work *for this instruction mix* — a different instruction mix (more
  memory operations, different arithmetic) can plateau at a different
  chain count entirely.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch8_dependency_chains.sh
```

This sweeps `--chains` from 1 to 16 for the same `--op=int` instruction
mix, tabulating throughput at each value.

**Expected qualitative result:** throughput should rise sharply at first,
then flatten well before reaching the highest chain count tested — not
keep climbing in proportion to the chain count. One example run on the
reference machine for this book (Apple M4, macOS, arm64) showed:

```text
chains   throughput_ops_s
1        317,058,995
2        576,450,166
4        852,665,021
8        1,247,342,575
12       1,250,231,784
16       1,249,415,104
```

Throughput approximately quadrupled from 1 to 8 chains, then stayed
essentially flat from 8 to 16.

**Interpretation:** the flattening point is specific to this CPU, this
instruction mix, and this run — do not expect the same chain count to be
where a different machine's curve levels off. The shape (rise, then
plateau) is the qualitative result; if your run shows a dip rather than a
clean plateau at the high end, that's plausible too (register pressure
from tracking many independent chains at once has its own cost) and
still consistent with this chapter's point: more independent work stops
being free at some point.

**Fallback path:** this lab has no external dependency beyond
`cyclelab` and `python3` (for parsing JSON); if `python3` is unavailable,
run the six `cyclelab compute --chains=...` commands directly and read
`results.throughput_ops_per_s` from each run's raw JSON.

**Cleanup:** none.

## Common Misconceptions

There is no blueprint-seeded misconception registry entry that fits this
chapter precisely; the closest is a specific case of M04 (Chapter 7):
**"A pipeline stall means the CPU is broken or misconfigured."** This is
wrong because stalls are a normal, expected consequence of dependency
chains and memory waits inherent to the *workload*, not a hardware fault —
a CPU with zero stalls ever would only be possible for a workload with
unlimited independent work, which essentially never occurs in practice.
The evidence that distinguishes the two: the same CPU, running the exact
same instruction mix, shows dramatically different stall behavior
depending purely on how independent the work is (this chapter's lab) —
nothing about the hardware changed between `--chains=1` and
`--chains=16`.

**"Adding more CPU cores or threads fixes a dependency-chain-limited
workload."** This is wrong because a single dependency chain running on
one thread is bound by that thread's own available instruction-level
parallelism, not by how many other cores exist on the machine — more
cores help independent *tasks* running in parallel, not one sequential
chain of dependent operations within a single thread. The evidence that
distinguishes the two: run `cyclelab compute --chains=1` with
`--threads=1` and again with a much higher `--threads` count on a
multi-core machine; per-thread throughput for that single dependency
chain does not improve, because the bottleneck was never a shortage of
cores — Chapter 21 returns to this distinction directly once threads and
scheduling are the chapter's subject rather than a single core's pipeline.

## Practical Implications

When a piece of code seems to be running unexpectedly slowly despite
looking computationally simple, ask whether its critical operations form
one long dependency chain rather than several independent ones — as in
this chapter's opening story, that shape alone can explain a large
performance gap between two functions with identical instruction counts.
Restructuring accumulation, reduction, or other sequentially-written
logic into independent partial results (summed at the end) is one of the
few genuinely general-purpose techniques for giving the pipeline more to
work with, though it isn't free — more independent state means more
registers or memory in use at once, a tradeoff worth being deliberate
about rather than applying reflexively.

## Key Takeaway

**A modern CPU is fast when it can keep many independent operations
moving; stalls are failures to supply that parallel work.**

## What to Remember

- A pipeline has a front end (fetch/decode) and a back end (execution
  units); a stall is a cycle where some part of it has nothing ready to
  work on.
- Out-of-order execution lets a CPU run instructions in an order other
  than program order, provided their inputs are ready.
- Dependency chains force strictly sequential execution regardless of how
  much unrelated, independent work sits nearby.
- Adding independent work increases throughput only up to the CPU's
  actual issue width and execution-unit capacity — past that, more
  independent work stops helping.
- The same instruction count and mix can perform very differently purely
  based on how dependent or independent the operations are.
- A stall is not a hardware fault; it's an expected consequence of a
  workload's dependency structure.
- Detailed, counter-based attribution of stalls to specific pipeline
  resources (top-down microarchitecture analysis) exists but requires
  vendor- and generation-specific tooling this chapter deliberately
  doesn't require.

## Further Reading

- Intel and AMD optimization manuals (architecture-specific; consult the
  manual matching your target CPU) — the primary sources for exact issue
  width, execution unit counts, and out-of-order buffer sizes per
  microarchitecture generation.
- Top-down microarchitecture analysis methodology — search vendor
  documentation for "TMA" or "Top-Down Analysis" for the counter-based
  version of this chapter's qualitative investigation.

## The Next Obvious Question

How do branches and dependencies disrupt execution?
