# The Pipeline: Front End, Back End, and Stalls

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
of this chapter's lab range. But that benefit has a ceiling, and the
ceiling shows up in two distinct, stacked ways, not one. First, a real
CPU has a finite issue width (how many instructions it can dispatch per
cycle), a finite number of execution units, and finite internal buffering
for tracking in-flight instructions — past some number of independent
chains, adding more doesn't create more usable parallelism, because the
CPU itself has nowhere to put the extra concurrency. Second, and less
obviously: each chain count this chapter's lab tests is a *different
compiled program* (Chapter 7's own compile-time specialization, one
function per chain count), and how efficiently the compiler can map that
specific number of accumulators onto the CPU's finite physical registers
does not scale smoothly with chain count — on this book's own reference
machine, throughput at 8 and 16 chains is noticeably higher than at 6,
10, 12, or 14, a real, reproducible pattern favoring chain counts that
happen to divide evenly into the machine's vector register width, not a
smooth plateau. Both mechanisms are real: **a stall is specifically a
failure to supply independent work**, and once independent work is no
longer the limiting factor, *how well the compiler can express that
independent work in hardware* becomes the next thing that matters —
which is why the shape past the initial rise is uneven rather than flat.

## Tool View

- What is measured: throughput as a function of available independent
  work (`--chains`), as an indirect but real signal of pipeline behavior.
- What is not measured: which *specific* resource (issue width,
  execution unit count, reorder buffer size, or the compiler's own
  register allocation for that specific chain count) is responsible for
  any one chain count's exact result — that requires either
  microarchitecture-specific hardware counters this chapter deliberately
  doesn't require, or reading the generated assembly for a specific
  chain count directly (`cc -S`, the same technique Chapter 6 used) —
  both reasonable next steps for a reader who wants the specific
  mechanism, not something this chapter's portable lab needs to already
  answer.
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

**Expected qualitative result:** throughput should rise sharply at
first, then — rather than settling into a clean, flat plateau — become
noticeably uneven as chain count keeps increasing, with some chain
counts clearly outperforming their neighbors. One example run (9
repetitions per chain count; medians shown, all individually
reproducible to within about 1%) on the reference machine for this
book (Apple M4, macOS, arm64) showed:

```text
chains   throughput_ops_s
1        727,005,494
2        1,317,117,343
4        2,339,897,373
6        1,507,949,372
8        2,235,614,622
10       1,820,018,479
12       1,745,995,332
14       1,840,566,430
16       2,561,270,716
```

Throughput roughly tripled from 1 to 4 chains, as expected. But it does
not then plateau: 6 chains is *slower* than 4, 8 recovers to nearly the
4-chain level, 10 and 12 drop again, and 16 — the highest chain count
tested — is the fastest result in the whole table. This specific run is
highly reproducible on this machine (repeating it lands within about 1%
every time), so this isn't noise; it's a real, second pattern on top of
the first one.

**Interpretation:** the initial rise (1 to 4 chains) is this chapter's
core lesson working exactly as expected: more independent work, more of
the pipeline kept busy. The uneven shape from 6 chains onward is the
*second* mechanism the Technical Explanation section names: each chain
count is a separately compiled, specialized function (Chapter 7), and
how efficiently the compiler can fit that many accumulators into the
CPU's physical registers and vector units doesn't scale smoothly with
chain count — on this build, chain counts that divide evenly into the
hardware's vector width (4, 8, 16) noticeably outperform the ones that
don't (6, 10, 12, 14). Do not expect the same specific numbers, or even
the same favored chain counts, on a different CPU or compiler — the
portable lesson is that *past the initial ILP-driven rise*, throughput
depends on compiler and hardware specifics that a black-box throughput
sweep can observe but not attribute; reading the generated assembly for
a couple of chain counts (Tool View, above) is the natural next step for
anyone who wants to know *why* their own machine favors the counts it
does.

**Fallback path:** this lab has no external dependency beyond
`cyclelab` and `python3` (for parsing JSON); if `python3` is unavailable,
run the nine `cyclelab compute --chains=...` commands directly, several
times each, and compare `results.throughput_ops_per_s` across
repetitions and chain counts by eye.

**Cleanup:** none.

## Common Misconceptions

### *"A pipeline stall means the CPU is broken or misconfigured." (a specific case of M04)*

**Why it's wrong:** Stalls are a normal, expected consequence of
dependency chains and memory waits inherent to the *workload*, not a
hardware fault — a CPU with zero stalls ever would only be possible for
a workload with unlimited independent work, which essentially never
occurs in practice.

**Correct intuition:** The same CPU, running the exact same instruction
mix, shows dramatically different stall behavior depending purely on how
independent the work is (this chapter's lab) — nothing about the
hardware changed between `--chains=1` and `--chains=16`.

**Analogy:** A single cashier isn't "broken" when a line forms — the
line is a normal consequence of customers arriving faster than one
register can serve them, not evidence the register is malfunctioning.

### *"Adding more CPU cores or threads fixes a dependency-chain-limited workload."*

**Why it's wrong:** A single dependency chain running on one thread is
bound by that thread's own available instruction-level parallelism, not
by how many other cores exist on the machine — more cores help
independent *tasks* running in parallel, not one sequential chain of
dependent operations within a single thread.

**Correct intuition:** Run `cyclelab compute --chains=1` with
`--threads=1` and again with a much higher `--threads` count on a
multi-core machine; per-thread throughput for that single dependency
chain does not improve, because the bottleneck was never a shortage of
cores — Chapter 21 returns to this distinction directly once threads and
scheduling are the chapter's subject rather than a single core's
pipeline.

**Analogy:** Hiring nine more people to help bake one cake doesn't speed
up the part where it has to sit in the oven for forty minutes — some
steps are strictly sequential, and adding more hands only helps the
steps that can actually happen in parallel.

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
