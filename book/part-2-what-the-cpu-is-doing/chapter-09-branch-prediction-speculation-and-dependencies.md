# Branch Prediction, Speculation, and Dependencies

**Part:** Part II — What the CPU Is Doing
**Concept level:** 2
**Prerequisites:** front end, back end, out-of-order execution, stall, dependency (Chapter 8)
**New concepts:** branch, branch predictor, misprediction, speculative execution, dependency chain, branchless trade-off

## Opening Question

How do branches and dependencies disrupt execution?

## Incident or Real-World Story

A team notices that a filtering step in their data pipeline — a simple
loop that checks a condition on each record and routes it one of two
ways — runs noticeably faster on some input files than others, even
though every file has roughly the same number of records and the same
proportion of records passing the filter. Profiling doesn't turn up
anything odd about the code itself; it's the same loop every time. The
difference turns out to be *order*: one file's records happen to arrive
already grouped by whatever property the filter checks — long runs of
records that pass, followed by long runs that don't — while another
file's records are effectively shuffled with respect to that property.
Same code, same total pass/fail ratio, same instruction count per
record. Only the *sequence* of outcomes differs, and that alone accounts
for the gap.

The mechanism is exactly the one this chapter names directly: the CPU
doesn't wait to find out which way a conditional branch will go before
starting to execute the next instructions. It guesses, based on recent
history, and starts executing down the guessed path — a real, useful
optimization that only costs something when the guess turns out wrong.
Grouped, repetitive outcomes are easy to guess correctly, over and over.
Shuffled ones are not, and every wrong guess has a real cost: work
already started down the wrong path has to be thrown away and restarted
correctly.

## Predict Before Measuring

Before reading further: given what Chapter 8 showed about independent
work versus dependency chains, predict how a *branch's* predictability —
not its dependency structure — might interact with that same pipeline.
If a conditional branch is easy to predict correctly almost every time,
do you expect the CPU to treat the code after it as effectively
independent, dependent, or does predictability not matter to that
question at all? Hold your answer for the Technical Explanation section.

## Worked Example

Three variations on the same underlying idea, one of which this
chapter's lab measures directly and two of which are worth reasoning
through even without a dedicated measurement:

- **Sorted versus random data through the same conditional.** Exactly
  this chapter's opening story, and its Guided Lab: identical code,
  identical total number of "true" and "false" outcomes, differing only
  in whether those outcomes arrive in predictable runs or shuffled.
- **One accumulator versus several independent accumulators.** This is
  Chapter 8's lab, revisited from a different angle: a dependency chain
  and a branch both create a kind of uncertainty the pipeline has to
  resolve one way or another — a dependency chain by waiting, a branch by
  guessing. They are different mechanisms with a shared consequence:
  both can leave execution units idle when the guess or the wait doesn't
  pay off.
- **A branchless rewrite that performs extra work and is not always
  faster.** Replacing `if (v >= 128) sum_high += v; else sum_low += v;`
  with a branchless version — computing both `sum_high += v` and
  `sum_low += 0` (or vice versa) unconditionally, using arithmetic or a
  conditional-move instruction instead of a jump — removes the
  possibility of a misprediction entirely, at the cost of *always* doing
  both computations instead of only the needed one. Whether that trade
  is worth it depends entirely on how expensive the extra unconditional
  work is compared to how often the original branch would have been
  mispredicted. For highly predictable data (this chapter's "sorted"
  case), the branch was nearly free already, and a branchless rewrite can
  easily lose by doing guaranteed extra work to avoid a misprediction
  that was rarely happening anyway.

## Core Intuition

A **branch** is any instruction whose next instruction depends on a
runtime condition — an `if`, a loop check, a `switch`. A **branch
predictor** is dedicated CPU hardware that guesses which way a branch
will go, based on that branch's recent history, before the condition is
actually known — the canonical picture is choosing a route before
reaching a fork in the road, based on which way paid off recently, and
being prepared to double back if this particular choice turns out wrong.
**Speculative execution** is the CPU acting on that
guess: fetching, decoding, and starting to execute instructions down the
predicted path *before* confirming it was right. A **misprediction** is
finding out the guess was wrong, which forces the CPU to discard whatever
speculative work it started and restart down the correct path — real,
wasted cycles, not a free guess. A **dependency chain** (Chapter 8) and
branch prediction are related but distinct sources of pipeline
uncertainty: a dependency chain is *known* in advance to require waiting;
a branch's outcome is *unknown* in advance and is instead predicted, at
the risk of being wrong.

## Technical Explanation

Predictable branches are nearly free for a different reason than
independent dependency chains are fast: a well-predicted branch lets the
CPU keep speculatively executing down the (correctly) guessed path
without ever having to stall waiting to find out which way to go — from
the pipeline's perspective, a correctly predicted branch barely disrupts
the flow of work at all. An unpredictable branch forces the opposite:
frequent mispredictions mean frequently discarding in-flight speculative
work and restarting, which shows up as real lost throughput even though
the *amount* of useful work (the actual arithmetic on each record) never
changed — exactly this chapter's opening story and its Guided Lab.

This is also why the branchless trade-off in the Worked Example doesn't
have a universal answer. A branch that's mispredicted often is expensive
to keep; replacing it with unconditional work that's always paid,
win or lose, can be a clear improvement. A branch that's predicted
correctly almost every time is already cheap; replacing it with
unconditional extra work adds guaranteed cost to remove a penalty that
was rarely being paid in the first place. The right choice depends on
the data's actual predictability — which is an empirical question, not
something to assume in either direction.

## Tool View

- What is measured: elapsed time and throughput for the same conditional
  logic, walking data in a predictable order versus an unpredictable one.
- What is not measured: the actual number of branch mispredictions —
  that requires a hardware counter (`branch-misses` in `perf stat`
  terms), which this chapter's portable lab doesn't require but the
  commands below show for a reader with access to it.
- Required permissions: none for the portable lab; counter access
  permissions (Linux) apply to the `perf stat` commands below, as in
  Chapter 7.
- Likely overhead: negligible for the portable lab.
- Portability: the portable lab works anywhere `cyclelab` runs. On
  Linux with counter access, the following (documented, not tested on
  this book's macOS reference machine) shows branches and misses
  directly:

  ```bash
  perf stat -e task-clock,instructions,branches,branch-misses -- \
    ./labs/cyclelab/bin/cyclelab branch --duration=2 --threads=1 --pattern=sorted
  perf stat -e task-clock,instructions,branches,branch-misses -- \
    ./labs/cyclelab/bin/cyclelab branch --duration=2 --threads=1 --pattern=random
  ```

  Compare the `branch-misses` count and the reported miss percentage
  between the two runs.
- Common failure mode: assuming a branchless rewrite is a strict
  improvement without first checking how predictable the original branch
  actually was.

## Guided Lab

**Portability:** portable (primary lab); the `perf stat` commands above
are **hardware-dependent** / Linux-only.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch9_branch_prediction.sh
```

This runs `cyclelab branch` with `--pattern=sorted` and
`--pattern=random`, same table size, same conditional, and compares
throughput.

**Expected qualitative result:** `sorted` should show substantially
higher throughput than `random`. One example run on the reference
machine for this book (Apple M4, macOS, arm64, 2,000,000-element table)
showed:

```text
pattern    throughput_elements_s
sorted     1,412,292,538
random     433,546,510
```

Roughly a 3.25x difference, from data order alone — the conditional
itself, and the total amount of arithmetic performed, are identical
between the two runs.

**Interpretation:** do not expect this exact ratio on a different
machine — branch predictor design varies significantly across CPU
generations and vendors, and this book deliberately keeps that detail
out of the main prose (see the style guide's architecture-portability
rule). The direction and rough order of magnitude (a clear, substantial
win for predictable data) is the qualitative result to look for.

**Fallback path:** if `python3` isn't available, run the two
`cyclelab branch --pattern=...` commands directly and read
`results.throughput_elements_per_s` from each run's raw JSON.

**Cleanup:** none.

## Common Misconceptions

### *"A branchless implementation is always faster."*

**Why it's wrong:** Removing a branch replaces an occasional
misprediction cost with a guaranteed, unconditional cost — worthwhile
only when the original branch was actually expensive (frequently
mispredicted), and a net loss when it wasn't.

**Correct intuition:** Measure the original branch's actual
predictability on real data (or, lacking a misprediction counter,
measure elapsed time under the realistic data distribution) before
assuming a branchless rewrite will win; this chapter's lab's `sorted`
case is exactly a scenario where the branch was already nearly free, and
a branchless rewrite there would have added guaranteed cost to avoid a
penalty that was rarely being paid.

**Analogy:** Always taking the stairs to avoid the small chance an
elevator is slow only makes sense if the elevator actually is
frequently slow — if it's usually fast, you've traded an occasional
minor wait for a guaranteed climb every single time.

### *"Sorting data specifically to help the branch predictor is always worth the sorting cost."*

**Why it's wrong:** Sorting has its own real cost — at best on the order
of *n* log *n* comparisons — and for a single linear pass over the data,
that cost can easily exceed whatever misprediction penalty it saves;
it's only clearly worthwhile when the same sorted order gets reused
across many passes.

**Correct intuition:** Compare total time for "sort, then scan once"
against "scan once, unsorted" on the same data — the sorted version can
lose overall even though its scan phase alone is measurably faster, once
the sort's own cost is counted.

**Analogy:** Alphabetizing your bookshelf before finding one book isn't
worth it — the time spent sorting exceeds the time saved searching,
unless you're going to search that same shelf many times afterward.

## Practical Implications

Before rewriting a hot conditional to be branchless, check whether the
condition's actual runtime data is predictable or not — the technique
only pays for itself against genuinely unpredictable branches. And more
generally: if a piece of code's performance varies a lot across
different inputs despite executing "the same logic," data order and
predictability (of both dependencies and branches) are worth checking
before assuming something is wrong with the code itself.

## Key Takeaway

**The pipeline performs best when it can predict control flow and find
independent work; removing a branch helps only if the replacement costs
less than the uncertainty.**

## What to Remember

- A branch predictor guesses which way a conditional will go before the
  condition is actually evaluated, so the CPU can keep executing instead
  of waiting.
- Speculative execution is the CPU acting on that guess; a misprediction
  means discarding that speculative work and restarting.
- Predictable branches (long runs of the same outcome) are nearly free;
  unpredictable ones carry a real, measurable cost from misprediction
  recovery.
- Dependency chains (Chapter 8) and branch mispredictions are different
  mechanisms that both disrupt the pipeline by leaving execution units
  without confirmed, ready work.
- A branchless rewrite trades an occasional misprediction cost for a
  guaranteed unconditional cost — a win only when the original branch
  was genuinely expensive.
- The same code, same instruction count, same total outcome proportions
  can perform very differently purely based on the *order* outcomes
  arrive in.
- Branch-misprediction counters (`branch-misses` via `perf stat` on
  Linux) make this effect directly measurable when available; elapsed
  time under realistic data is a portable substitute.

## Further Reading

- Linux perf manual pages: <https://man7.org/linux/man-pages/man1/perf.1.html>
- Intel and AMD optimization manuals (architecture-specific) — primary
  sources for exact branch predictor behavior per microarchitecture
  generation, deliberately kept out of this chapter's main prose.

## The Next Obvious Question

How can counters turn a vague slowdown into a hypothesis?
