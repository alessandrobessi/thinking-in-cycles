# The Performance Investigation Loop

**Part:** Part I — Measuring Reality
**Concept level:** 0 (informally previews Level 7 "bottleneck shift" and "causal claim," formalized in Chapter 30)
**Prerequisites:** workload, metric, latency, throughput, distribution, confounder (Chapters 1-4)
**New concepts:** baseline, hypothesis, falsification, scope, perturbation, causal claim, bottleneck shift

## Opening Question

What investigation process prevents random tuning?

## Incident or Real-World Story

A team is told a batch job "got slower." Someone increases the thread
pool size, because more threads usually means more parallelism. The job
finishes in about the same time. Someone else, on a separate afternoon,
tries switching a data structure to one with better theoretical
complexity. No measurable change. A third person adds caching to a
function that looked expensive in a quick read of the code. Also no
change — if anything, slightly slower, because now there's cache-management
overhead on a function that wasn't the problem. Three engineer-days pass.
The job is still slow, and now the codebase has three unexplained changes
in it, at least one of which is a net negative.

Nobody in this story did anything unreasonable in isolation — more
threads, better complexity, and caching are all legitimate techniques.
What was missing was a process: none of the three changes started from a
measurement of *where the job's time was actually going*, and none of
them were evaluated by rerunning the same controlled comparison before
moving to the next idea. Each was a guess, tried once, on an
uncontrolled machine, with no baseline to fall back to and no hypothesis
that could have been proven wrong. The job was still slow at the end not
because the problem was unsolvable, but because guessing had replaced
investigating.

## Predict Before Measuring

Before reading further: if you were only allowed to change **one** thing
about this story's investigation — process, not code — which one change
would have had the biggest effect on whether the team found the real
answer? Hold that answer; the rest of this chapter names exactly that
process.

## Worked Example

The loop this chapter introduces, applied to the story above, would have
looked like this instead:

1. **Define the workload and success metric** — this batch job,
   processing this dataset, judged by total completion time.
2. **Reproduce the problem** — confirm it's still slow, on demand, not
   just "someone said it was slow last week."
3. **Establish a baseline** — run the *current, unchanged* job several
   times, interleaved-consistent with Chapter 4, and record the
   distribution of completion times.
4. **Classify where time is going** — before touching any code, is the
   job spending its time on-CPU, waiting on I/O, blocked on a lock, or
   something else? (Chapter 1's accounting question, asked first.)
5. **Form one hypothesis** — a specific, falsifiable guess: "most of the
   added time is spent waiting on the database connection pool," not
   "something about concurrency."
6. **Choose the least invasive measurement that can test it** — the
   cheapest tool that could show the hypothesis is wrong, not the most
   powerful one available.
7. **Change one thing** — a single, specific change motivated directly by
   the hypothesis.
8. **Rerun the controlled experiment** — same baseline conditions,
   interleaved with the unchanged version if possible.
9. **Check for bottleneck movement and regressions** — did the metric
   that mattered actually improve, and did anything else get worse?
10. **Document the evidence and uncertainty** — what was measured, what
    it showed, what it didn't prove.

Three changes tried once each, with no baseline and no hypothesis, is not
a shorter version of this loop. It's a different activity that happens to
also involve editing code.

## Core Intuition

A **baseline** is a controlled reference measurement — the state you can
always return to and compare against — established *before* any change,
using the same hygiene from Chapter 4 (repetition, interleaving where
possible). A **hypothesis** is a specific, testable guess about where time
is going or what a change will do, stated before measuring, not
constructed afterward to fit whatever happened. **Falsification** means
designing a measurement that *could* show the hypothesis is wrong, not
only ones that could confirm it — "I expect this metric to drop by roughly
X if the hypothesis is right" is falsifiable; "I'll know it if I see it"
is not. **Scope** is the explicit boundary of what a measurement or claim
covers — this workload, this machine, this input — and, by implication,
what it doesn't. **Perturbation** is any change the act of measuring
itself introduces into the thing being measured; even a lightweight timer
has some cost, and an invasive profiler can have much more.

## Technical Explanation

Two ideas from the loop are worth making explicit because they're easy to
skip under time pressure.

**One change at a time is not a stylistic preference; it's what makes
step 9 possible at all.** If three things change together and the metric
improves, there is no way to know which of the three mattered, whether
one helped while another hurt, or whether the "improvement" was really
just favorable noise from Chapter 4's variance. Attribution requires
isolation.

**A fixed bottleneck can move, not disappear, once you fix it —** a
**bottleneck shift**. Removing the lock contention that was limiting a
service might reveal that the database connection pool was the *next*
constraint all along, just hidden behind the lock. This is not a failure
of the fix; it's the expected shape of a real investigation, and it's why
step 9 explicitly asks about movement, not just about whether the metric
you were watching improved. A **causal claim** — "this change caused this
improvement" — is only earned once a change has been isolated, measured
under the same controlled conditions as the baseline, and shown to move
the metric beyond what run-to-run variance alone would explain.

## Tool View

At this stage, deliberately, no specialized tool is needed — the loop's
whole point is that steps 1 through 6 can be done with only what Chapters
1 through 4 already introduced: wall/user/system time, throughput at
varying load, and repeated, interleaved comparisons.

- What is measured: enough to test one falsifiable hypothesis, no more.
- What is not measured: anything not motivated by the current hypothesis
  — resisting the urge to "just check everything" with every powerful
  tool available is itself part of the discipline (Section 15's tool
  ladder, introduced later, formalizes this as the least-invasive-tool
  principle).
- Required permissions: none, at this level of the tool ladder.
- Likely overhead: the loop costs *time*, mostly in establishing a proper
  baseline — that cost is the loop's main practical objection, and its
  main defense is that it's cheaper than three unexplained changes and
  three engineer-days.
- Common failure mode: skipping straight to step 7 (change one thing)
  without steps 3-6, which is exactly this chapter's opening story.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch5_investigate_slow_config.sh
```

This runs `cyclelab compute` with a deliberately over-threaded
configuration (four times as many worker threads as logical CPUs) next
to a correctly-sized one, and prints the `ps`/`vmstat` (or `vm_stat` on
macOS) commands needed to classify where time is going, walking through
steps 1-10 of the loop.

**Expected qualitative result:** total throughput between the
over-threaded and correctly-sized runs should be similar, not
proportional to the extra thread count — the CPUs can't do more total
work just because more threads are asking for it. One example run on the
reference machine for this book (10 logical CPUs, 40 vs. 10 threads,
2-second runs) showed:

```text
over-threaded (40 threads): total_iterations=468,332,544  throughput=1,872,943,413 ops/s
correctly-sized (10 threads): total_iterations=467,045,376  throughput=1,868,067,552 ops/s
```

Total throughput differed by under 0.3% despite a 4x difference in
thread count. See
`labs/expected-shapes/ch05-investigation-loop-shape.md` for the full
expected shape, including what to look for in `ps`/`vmstat` output.

**Interpretation:** if total throughput is roughly flat across both
configurations on your machine too, that's this chapter's central claim
made concrete: more threads than CPUs doesn't create more capacity, it
creates more competition for the same capacity — the "runnable but not
running" distinction Chapter 21 will formalize, reached here using only
the investigation loop and tools already introduced.

**Cleanup:** none.

**Fallback path:** the script uses `python3` to format `cyclelab`'s JSON
output and `vmstat`/`vm_stat` to show system-level activity; if none of
these are available, run the two `cyclelab compute --threads=...`
commands directly, read `results.total_iterations` and
`results.throughput_ops_per_s` from the raw JSON, and use `ps` alone
(available essentially everywhere) for step 4's classification.

## Common Misconceptions

**M23 (proposed) — "Changing several things at once and observing an
improvement proves which change mattered."** This is wrong because
without isolating one change at a time and testing a falsifiable
hypothesis, an improvement after a multi-part change cannot be attributed
to any specific part of it — one part could even be masking a regression
in another. The evidence that distinguishes the two: revert changes one
at a time (or apply them one at a time from baseline) and re-measure; if
the ranking of "which single change explains the improvement" comes out
undefined or contradictory, the original multi-change comparison wasn't
diagnostic in the first place.

## Practical Implications

Before making any performance change, write the hypothesis down first, in
a form specific enough that a measurement could prove it wrong. If a
proposed fix can't be stated that way — "this should help because X" with
a specific, checkable X — that's a sign the investigation hasn't reached
step 5 yet, no matter how confident the proposed fix feels.

## Key Takeaway

**Performance work advances by eliminating explanations, not by
accumulating tuning ideas.**

## What to Remember

- A baseline, established with Chapter 4's hygiene, is what every later
  comparison in an investigation gets measured against.
- A hypothesis must be specific enough that a measurement could show it's
  wrong — falsifiability is what separates investigating from guessing.
- Classify where time is going (Chapter 1's on-CPU/off-CPU accounting)
  before picking a tool, and pick the least invasive tool that can test
  the current hypothesis.
- Change one thing at a time; without isolation, attribution is
  impossible even when the metric improves.
- A fixed bottleneck can shift to reveal the next one — checking for that
  movement is part of the loop, not an optional extra step.
- A causal claim needs a controlled, interleaved comparison against
  baseline, not just "it ran once and looked better."
- Document what the evidence shows and what it doesn't — scope matters as
  much as the result.

## Further Reading

- This chapter's ten-step loop is the book's canonical statement of the
  investigation process, reused without modification in every later
  chapter's Guided Lab and in the Chapter 30 case study.
- `templates/performance-report-template.md` — the report structure this
  loop's documentation step (10) produces.

## The Next Obvious Question

What work does the CPU actually execute?
