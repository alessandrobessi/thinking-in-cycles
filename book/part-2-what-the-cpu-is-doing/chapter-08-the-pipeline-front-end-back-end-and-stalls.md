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
of this chapter's lab range. But that benefit has a ceiling: a real CPU
has a finite issue width (how many instructions it can dispatch per
cycle), a finite number of execution units, and finite internal buffering
for tracking in-flight instructions — past some number of independent
chains, adding more doesn't create more usable parallelism, because the
CPU itself has nowhere to put the extra concurrency.

Past that initial rise, this chapter's lab result is uneven rather than
flat — chain counts 4, 8, and 16 (this lab's tested values above 2) do
not form a smooth plateau; 8 measures a little *below* 4 on this
reference machine, and 16 measures above both. An earlier draft of this
chapter attributed a similar-looking pattern, measured across a wider
range of chain counts, directly to the machine's vector register width —
that attribution turned out to be wrong: it was largely an artifact of
this lab's own fixed 16-slot-per-iteration schedule, which distributes
updates unevenly across chains whenever the chain count doesn't divide
16 evenly, giving some chain counts a longer critical path than others
for reasons that have nothing to do with the hardware. That's exactly
why this lab restricts itself to chain counts that *do* divide 16 evenly
(1, 2, 4, 8, 16) — the one honest way to compare chain counts fairly
with a fixed-size (16 update slots per iteration) schedule is to only
test values where every chain gets the identical number of updates. The remaining unevenness among 4,
8, and 16 is real and reproducible even with that confound removed, but
this chapter's portable lab has no way to attribute it to a specific
cause (register allocation, code layout, and several other
compiler-specific choices are all plausible candidates) — reading the
generated assembly for each specialization (Tool View, below) is the
honest next step for a reader who wants the specific mechanism, not
something to guess at from a throughput number alone. **A dependency
stall specifically occurs when the processor cannot find independent,
ready work to execute while a required result is still pending** — one
named cause among the several Core Intuition already lists (a memory
wait is a different one), not a definition of stalls in general.
Chapter 8's central lesson survives fully intact: independent work
matters, up to the hardware's real capacity — but exactly where a
specific chain count lands past that capacity is sensitive to compiler
and hardware specifics this black-box measurement cannot by itself
untangle.

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

This sweeps `--chains` over 1, 2, 4, 8, and 16 for the same `--op=int`
source-level update workload, tabulating throughput at each value. Those five values
are deliberately not "1 through 16" — each one unrolls into exactly 16
slots per iteration (Chapter 7), and only chain counts that divide 16
evenly (1, 2, 4, 8, 16) give every chain the identical number of updates
per iteration. A chain count that doesn't divide 16 evenly (6, for
instance: sixteen slots split six ways gives some chains three updates
and others two) hands one or more chains a longer critical path than the
rest for reasons that have nothing to do with the hardware — a confound
this lab avoids by construction rather than needing to explain away.

**Expected qualitative result:** throughput should rise as chain count
increases from 1, and those gains should taper off — that part is
portable. The exact shape past that point (a clean plateau, a gradual
decline, something uneven) is *not* itself a portable expectation; it
depends on the specific compiler and microarchitecture, and this
black-box throughput sweep can observe whatever shape shows up without
being able to explain it. This script collects each round's five
repetitions (one per chain count) in a freshly randomized order every
round, not a fixed rotation and not run in blocks — a *fixed* rotating
order would still confound chain count with whichever position always
runs first, last, and so on within each round, in addition to the
sweep-wide drift a blocked order confounds it with. Randomizing the
order fresh each round is what substantially reduces the risk of both,
the same logic as a randomized block design generally, applied here to
a five-way sweep instead of Chapter 4/15's two-way before/after. Fresh
randomization is not a guarantee, though: with only nine rounds and
five positions, perfect positional balance isn't achievable, and by
chance one chain count could still land in a favorable position more
often than another across a specific set of rounds — the honest claim
is "collection order is very unlikely to explain the ranking below,"
not "collection order is proven not to." One example run
(9 randomized-order repetitions per chain count; medians shown, all
individually reproducible to within about 1-2%) on the reference
machine for this book (Apple M4, macOS, arm64) showed:

```text
chains   throughput_ops_s
1        702,656,358
2        1,299,768,237
4        2,329,861,466
8        2,216,408,833
16       2,546,049,694
```

Throughput roughly tripled from 1 to 4 chains, as expected. On this
particular machine and compiler, it does not then plateau cleanly: 8
chains measures a little *below* 4, and 16 — the highest chain count
tested — is the fastest result in the table. This specific ranking
repeats here across multiple full sweeps, each with its own fresh
random ordering, so it isn't run-to-run noise, and a fixed collection
order is very unlikely to be the explanation, since the order itself is
different every time it reappears. That's evidence against a
collection-order artifact, not a proof against one — a residual,
by-chance positional imbalance across a specific set of nine rounds
remains possible in principle, which is exactly why the Interpretation
below stops short of attributing the pattern to any specific cause. It
also isn't the earlier schedule-fairness artifact, since 4, 8, and 16
all divide 16 evenly. It's a real, second pattern on top of the first
one — specific to this machine and compiler, not a general property of
out-of-order CPUs.

**Interpretation:** the initial rise (1 to 4 chains) is this chapter's
core lesson working exactly as expected: more independent work, more of
the pipeline kept busy. The remaining unevenness among 4, 8, and 16 is
not explained by this lab's earlier schedule-fairness bug (all three
divide 16 evenly) and is very unlikely to be a collection-order
artifact (Guided Lab, above) — but "very unlikely to be an artifact of
measurement" is not the same claim as "proven to be caused by the
hardware or the compiler," and this portable, black-box throughput
sweep has no way to say *why*
— register allocation, code layout, and other compiler-specific choices
are all plausible candidates, and a specific answer would need to come from
reading the generated assembly for these three specializations directly
(Tool View, above), not from the throughput numbers alone. Resist the
temptation to reach for a hardware explanation (vector width, cache
behavior, or similar) without that assembly-level confirmation — a
similar-looking pattern measured across a wider, schedule-unfair range
of chain counts once led this very book to the wrong explanation, which
is exactly why this lab now restricts itself to a fairness-controlled
set in the first place. Do not expect the same specific numbers, or the
same relative ranking of 4 vs. 8 vs. 16, on a different CPU or compiler
— the portable lesson is that *past the initial ILP-driven rise*,
throughput depends on specifics a black-box sweep can observe but not
attribute.

**Fallback path:** this lab has no external dependency beyond
`cyclelab` and `python3` (for parsing JSON); if `python3` is unavailable,
run the five `cyclelab compute --chains=...` commands directly, several
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

**Correct intuition:** The same CPU, running the same source-level
update workload (Chapter 7's own caveat: `--chains=1` and `--chains=16`
are separately compiled specializations, so "exact same instruction
mix" is a machine-code-level claim this book doesn't verify), shows a
dramatically different *throughput* when the only *source-level* change
is how independent the requested work is (this chapter's lab) — a
result *consistent with* dramatically different stall behavior, though
this portable lab measures throughput, not stalls directly, and cannot
by itself rule out the compiled code contributing to the difference
alongside dependency structure; nothing about the hardware changed
between the two runs.

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
moving; a dependency stall is what happens when it can't find any.**

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
- The same source-level operation sequence can perform very differently
  depending on how dependent or independent the operations are — the
  machine-code-level instruction mix isn't verified to be identical
  (Chapter 7), so this is what the controlled source change shows, not
  a claim about the compiled binaries.
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
