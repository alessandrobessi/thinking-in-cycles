# Chapter 4 — Noise, Variance, and Honest Benchmarks

**Part:** Part I — Measuring Reality
**Concept level:** 0
**Prerequisites:** workload, metric, latency, throughput (Chapters 1-3)
**New concepts:** warm-up, repetition, distribution, outlier, randomization, confounder, thermal state, frequency scaling, background interference, confidence interval intuition

## Opening Question

How do we know a measured difference is real?

## Incident or Real-World Story

An engineer benchmarks a proposed optimization: run the old code once, run
the new code once, compare the two numbers. The new code is 6% faster.
They open a pull request citing the number. A reviewer, skeptical, runs
both versions five times each, back to back — old code five times, then
new code five times — and finds the "6% faster" result only shows up in
about half the pairs; the other half shows the two versions within a
percent of each other, sometimes with the old code slightly ahead.

The reviewer digs further and finds two compounding problems. First, the
laptop used for the original benchmark had been idle for a while
beforehand, then ran hot partway through testing the new code, and the
CPU quietly reduced its clock speed to manage heat — a real effect, but
one that had nothing to do with the code change. Second, because the old
code was always run first and the new code always second, *any* systematic
drift over time — thermal, or a background process warming up, or the
machine's own scheduler settling into a pattern — would automatically look
like it favored one version over the other, regardless of which version
was actually faster. The 6% number wasn't fabricated. It was one sample
from a process nobody had checked was even stable enough to compare.

## Predict Before Measuring

Before reading further: if you run the *exact same* unchanged program ten
times in a row on an otherwise idle machine and record its throughput each
time, do you expect all ten numbers to be identical? If not, roughly how
much spread would surprise you — under 1%, a few percent, tens of
percent? Hold that expectation; the Guided Lab will show you a real
distribution to compare it against.

## Worked Example

Take two nearly identical loops that should perform almost the same, and
benchmark them two different ways.

**Blocked order:** run loop A five times, record the numbers, then run
loop B five times, record those. Whatever the two averages come out to,
report "B is X% faster than A."

**Interleaved order:** run A, then B, then A, then B, five times each,
alternating, and record every individual result rather than only the
final averages.

The blocked version is the version most people run instinctively — it's
simpler to script. It is also the version most vulnerable to exactly the
failure mode in this chapter's story: any trend over time (the machine
warming up, another process starting, frequency scaling kicking in) gets
attributed entirely to "loop B is different from loop A," because time and
which-loop-is-running are perfectly correlated in the blocked design. The
interleaved version breaks that correlation: a time-based drift now shows
up as both loops trending the same direction over the course of the run,
which is visibly distinguishable from one loop being consistently ahead of
the other.

## Core Intuition

A single measurement is not a **distribution** — it's one draw from
whatever process actually generates your program's runtime, and that
process has real spread (**variance**) even when nothing about the
program changes between runs. **Repetition** — running the same
controlled configuration many times — is what turns "a number" into "a
distribution you can actually reason about." An **outlier** is a
measurement far outside the bulk of that distribution; it might be noise,
or it might be a real rare event, and the difference matters, but neither
possibility is settled by looking at only one run. A **confounder** is
anything other than the change you're testing that could explain an
observed difference — the blocked-order story above is confounding time
with configuration. **Randomization** (or, as in the worked example,
interleaving) is the standard defense: if you can't eliminate a
confounder, at least prevent it from correlating with the thing you're
trying to measure.

## Technical Explanation

Several real, physical, and OS-level effects contribute to run-to-run
variance, and a credible benchmark has to account for all of them, not
just "the computer is a bit noisy":

- **Warm-up.** Caches, memory allocators, and (in other languages) JIT
  compilers all start cold and reach a steady state after some initial
  work. The very first repetition of a benchmark is frequently the
  slowest one, for reasons that have nothing to do with the code being
  compared.
- **Thermal state and frequency scaling.** Modern CPUs raise and lower
  their clock speed based on temperature, sustained load, and power
  limits. A CPU that's been idle can briefly run faster ("boost") than
  the same CPU after minutes of sustained load, independent of what code
  it's executing.
- **Background interference.** Anything else running on the same
  machine — a backup job, a browser tab, another engineer's build —
  competes for the same CPU, cache, and memory bandwidth and can
  distort results unpredictably.
- **Confidence interval intuition.** Without needing the formal
  statistics, the informal version is enough for most performance work:
  a measured difference should be judged against how much repeated
  measurements of the *same, unchanged* configuration naturally vary. If
  two configurations' results overlap about as much as either
  configuration's own repeated results vary against themselves, you do
  not yet have evidence of a real difference.

None of this means benchmarking is hopeless — it means a benchmark is an
experiment, with the same obligations as any other controlled experiment:
warm up before timing, repeat enough to see a distribution, randomize or
interleave to break confounders, and report the spread, not just the
average.

## Tool View

- What is measured: the distribution of a metric (here, throughput) across
  repeated, interleaved runs of two configurations.
- What is not measured: statistical significance in any formal sense —
  this chapter builds intuition, not hypothesis testing; Appendix F is
  where the fuller statistical treatment lives.
- Required permissions: none.
- Likely overhead: repetition takes real wall-clock time; there is a
  genuine tradeoff between more confidence and more time spent measuring.
- Portability: works anywhere `cyclelab` and `python3` run.
- Common failure mode: running a comparison once, blocked (all of A, then
  all of B), and reporting the single resulting percentage as if it were
  a settled fact.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch4_interleaved_ab.sh 8 0.5
```

This runs `cyclelab compute --op=int` and `--op=float` interleaved, eight
repetitions each, printing every individual result rather than only an
average.

**Expected qualitative result:** see
`labs/expected-shapes/ch04-distribution-shape.md` for the full shape
description. In short: results from the *same* configuration should
cluster fairly tightly; a real difference between `int` and `float` looks
like two clusters that don't overlap much, not just two different
averages. One example run on the reference machine for this book (a
shorter, 3-repetition version) showed:

```text
rep    op       throughput_ops_s
1      int      292,504,145
1      float    323,858,243
2      int      336,393,299
2      float    323,550,027
3      int      292,290,599
3      float    323,622,231
```

Note that `int`'s three values (292M, 336M, 292M) spread out more than
`float`'s three values (324M, 324M, 324M) do — and one of `int`'s values
(336M) actually lands above two of `float`'s. With only three repetitions
per configuration, this is not enough evidence to confidently rank `int`
against `float`; more repetitions would be needed to see whether `int`'s
spread is real variance or a fluke of a small sample.

**Interpretation:** if your run's `int` and `float` columns overlap
similarly to this example, that is itself the lesson — a small number of
repetitions can leave you unable to distinguish "these are different" from
"this is the same distribution, sampled twice." Increase `REPS` and see
whether the picture clarifies.

**Cleanup:** none.

**Fallback path:** the script uses `python3` only to extract one field
from `cyclelab`'s JSON output. If `python3` isn't available, run the
`cyclelab compute` commands directly, alternating `--op=int` and
`--op=float` by hand, and read `results.throughput_ops_per_s` from each
run's raw JSON.

## Common Misconceptions

**M15 — "One benchmark run is evidence."** This is wrong because a single
run is an anecdote unless the effect is overwhelming and the environment
is tightly controlled — ordinary run-to-run variance can easily produce a
"6% faster" result between two runs of the *identical, unchanged* program.
The evidence that distinguishes the two: run the same configuration
repeatedly, interleaved with its comparison; if repeat runs of the *same*
configuration spread nearly as much as the two configurations differ, a
single run proves nothing.

**M20 (touched on) — "A profiler's output (or, here, a benchmark
harness's output) is ground truth."** Even a correctly-written benchmark
script is a measurement system with its own scope and blind spots — this
chapter's own harness only compares throughput, only on one machine, only
under one background-load condition, and says nothing about latency
variance, thermal history, or any confounder this chapter didn't think to
control for.

## Practical Implications

Before trusting any "X% faster" claim — your own or someone else's — ask
how many repetitions it's based on, whether the comparison was interleaved
or blocked, and whether the machine was otherwise idle. A number that
can't answer those three questions isn't wrong, but it isn't evidence yet
either.

## Key Takeaway

**A benchmark is an experiment, and uncontrolled experiments produce
confident stories about noise.**

## What to Remember

- A single measurement is one draw from a distribution, not the
  distribution itself.
- Warm-up, thermal state, frequency scaling, and background interference
  are all real sources of run-to-run variance, not excuses.
- Blocked-order comparisons (all of A, then all of B) let any time-based
  drift masquerade as a real difference between A and B.
- Interleaving or randomizing run order breaks that confound.
- Judge a measured difference against how much repeated runs of the
  *same* configuration vary — that's the informal confidence-interval
  check.
- Report spread, not just an average, when comparing two configurations.
- "It ran once and was faster" is not the same claim as "it is faster."

## Further Reading

- Georges, Buytaert, and Eeckhout, "Statistically Rigorous Java
  Performance Evaluation," OOPSLA 2007 — the canonical paper on why naive
  single-run benchmarking misleads, in a different language but the same
  underlying problem. See `references/bibliography.md`.
- Google Benchmark user guide:
  <https://google.github.io/benchmark/user_guide.html> — a production
  microbenchmarking harness that implements much of this chapter's
  hygiene (warm-up, repetition, statistics) directly.

## The Next Obvious Question

What investigation process prevents random tuning?
