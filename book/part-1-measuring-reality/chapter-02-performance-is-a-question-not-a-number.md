# Performance Is a Question, Not a Number

**Part:** Part I — Measuring Reality
**Concept level:** 0
**Prerequisites:** workload, resource, on-CPU, off-CPU, bottleneck, critical path (Chapter 1)
**New concepts:** operation, workload model, response time, completion time, service level, capacity, cost per unit of work

## Opening Question

What exactly does "faster" mean for this workload?

## Incident or Real-World Story

A team maintains a data-serialization library used in two very different
places: a nightly batch job that re-encodes a multi-terabyte archive, and
a live API that serializes one small record per request under strict
latency requirements. An engineer proposes a change that switches the
library's internal buffer strategy to allocate one large buffer up front
instead of growing it incrementally. They benchmark it against the batch
job's workload and the change is a clear win — total time to re-encode
the archive drops by 15%. The change ships.

Within a day, the API team reports a regression: p99 latency for single
small records got worse, not better. Both teams were using the same
library, the same change, and the phrase "we made it faster" — but they
meant two different, specific, measurable things, and the change helped
one at the expense of the other. The large up-front allocation amortizes
beautifully over millions of records in a batch job; for a single small
record, it's pure overhead paid on every request. Nobody was wrong about
their own measurement. The mistake was treating "faster" as if it named
one number instead of an answer to a specific question that has to be
asked for a specific workload.

## Predict Before Measuring

Before reading further: if a change makes a fixed-size buffer allocation
strategy replace a growing one, would you expect its effect on total
throughput to be the same at a small input size and a large input size?
If not, which direction do you expect the difference to go, and why? Hold
that prediction — the Guided Lab measures the same underlying *shape*
(a fixed, one-time cost competing against a cost that scales with how
much work one run does), using a different, more reproducible proxy for
"how much work" than an actual growing input.

## Worked Example

Three real workloads, three different definitions of "faster," each one
entirely legitimate for its own case:

- **A batch job** re-encoding an archive overnight cares about
  **completion time** — the elapsed time from starting the whole job to
  finishing every record in it. Nobody is waiting on any single record;
  they're waiting on the last one.
- **An API** serving live traffic at a roughly fixed request rate cares
  about **p99 response time** — the time from one request arriving to its
  result being ready, measured at the tail, because a small fraction of
  slow requests still burns real users even if the average looks fine.
- **A data pipeline** processing a continuous stream cares about
  **records per second within a fixed CPU budget** — its **capacity**,
  the maximum sustainable rate it can process without falling behind,
  given a resource ceiling it isn't allowed to exceed.

A change that improves one of these numbers can be neutral, or actively
harmful, to another — as the story above shows for exactly this reason.
None of the three teams is measuring performance "wrong." They are
measuring three different **operations** (one full archive job; one
single request; one record in a stream) against three different
**service levels** (finish overnight; keep p99 under a target; keep up
with the input rate on a fixed budget).

## Core Intuition

A **workload model** is an explicit description of what work is being
generated, at what rate, with what input characteristics — it is what
turns "the program" into something you can actually reason about. An
**operation** is the smallest unit that model's metric is defined over:
one record, one request, one batch job, depending on what you're
measuring. A **metric** only means something once you've said which
operation and which workload model it's attached to; "10,000 operations
per second" and "50ms p99 latency" can both be true of the same system
and are answers to different questions, not competing claims about the
same one.

**Cost per unit of work** — CPU-time, dollars, energy divided by useful
work completed — is a fourth lens worth naming here: sometimes "faster"
isn't even the goal; a system that does the same work using less of a
constrained resource has improved, even if elapsed time didn't change at
all.

## Technical Explanation

The library story generalizes into a rule, and the rule has a precise
shape: for a fixed, one-time cost `T_fixed` (a startup cost, paid exactly
once) and a marginal, per-unit cost `T_unit` (paid once per record,
iteration, or unit of work), total time for `N` units is

```
T(N) = T_fixed + N * T_unit
```

`T_fixed / N` — the fixed cost's *share* of the total — shrinks as `N`
grows, which is exactly why a large up-front buffer allocation is
invisible when amortized over millions of records and dominant when
there's only one record to amortize it over: the same `T_fixed` term,
divided by wildly different `N`. This isn't specific to buffers — the
same shape shows up in thread-pool warm-up, connection pooling, JIT
compilation, cache warming, and batch-size tuning of every kind. None of
these are bugs in the technique. They are properties of *which* workload
model you evaluate them against — specifically, of how large `N` is
relative to the ratio `T_fixed / T_unit` for that specific change.

This is also why **service level** has to be stated, not assumed. "Keep
p99 under 100ms" and "finish the batch by 6 a.m." are both legitimate
service levels, but a change evaluated against the wrong one will report
a false verdict — improved by one standard, regressed by the other,
simultaneously true.

## Tool View

No new measurement tool is needed yet — the tool from Chapter 1 (compare
wall time / user time / system time, or here, throughput at varying
measurement length) is enough, applied more deliberately:

- What is measured: a metric (here, throughput) at more than one
  measurement length (iteration count, this chapter's proxy for `N` in
  `T(N) = T_fixed + N * T_unit`), for more than one configuration, so a
  comparison can be made *within* each length rather than pooled across
  lengths.
- What is not measured: a genuine varying-*data*-size experiment — this
  lab varies how many times the same fixed-size operation repeats, not
  the size of an input structure. Chapters 17 and 19's `--working-set-size`
  sweeps are this book's actual varying-data-size experiments; use those
  as the template if the question is specifically about input size rather
  than measurement length.
- What is not measured, either way: which length or size is "the real
  one" — that's a decision about the workload model, not something a
  measurement can answer for you.
- Required permissions: none.
- Likely overhead: negligible.
- Portability: works anywhere `cyclelab` runs.
- Common failure mode: reporting a single benchmark number ("20% faster")
  without saying at what measurement length or input size it was
  measured, then being surprised when it doesn't generalize.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch2_size_sweep.sh
```

This runs `cyclelab compute` at three fixed iteration counts — a short,
medium, and long measurement length, standing in for `N` in
`T(N) = T_fixed + N * T_unit`, *not* a changing input size — with two
operation mixes (`--op=int` and `--op=mixed`), **nine repetitions per
cell**, reporting the median and the min-max range at each length
(Chapter 4's repetition discipline, borrowed a couple of chapters early
because it's already necessary here).

One unit caveat before reading any of these numbers: `--op=int` and
`--op=mixed` are not doing equal work per reported "op." Each `mixed`
op updates both an integer and a floating-point accumulator, while each
`int` op updates only the integer one — so `throughput_ops_per_s` is
meaningful for comparing the *same* op across measurement lengths, but
not for reading the raw gap between `int` and `mixed` as "how much
cheaper integer work is." That gap always includes the fact that
`mixed` is doing strictly more per op, on top of whatever else is
happening.

**Expected qualitative result:** at the shortest measurement length, the
spread across repetitions of the *same* configuration should be
comparable to, or larger than, the gap between `int`'s and `mixed`'s
medians — meaning a single run cannot reliably rank them at all. At
longer lengths, the two configurations' medians should settle into a
smaller, more stable gap. One example run on the reference machine for
this book (Apple M4, macOS, arm64, `cyclelab` release build, 9
repetitions per cell) showed exactly that pattern:

```text
iterations   op       median_ops_s   min_ops_s      max_ops_s
5000         int      526,315,813    283,687,891    547,945,402
5000         mixed    533,333,411    449,438,307    540,540,676
5000000      int      730,967,437    711,819,768    732,493,406
5000000      mixed    680,526,727    646,987,464    686,565,627
100000000    int      722,133,761    656,314,401    729,445,148
100000000    mixed    670,456,422    658,248,220    673,291,292
```

At 5,000 iterations, `int`'s own min-max spread (283.7M-548.0M, a range
of roughly 264M) is far *wider* than the 7M gap between `int`'s and
`mixed`'s medians — the two configurations' medians are, for practical
purposes, tied, and any single one of these nine runs could have shown
either one "ahead." At 5,000,000 and 100,000,000 iterations, the
medians separate to a stable ~50M-ops/s gap (`int` consistently ahead,
by roughly 7-8%) at both lengths, and the two configurations' min-max
ranges barely overlap — a genuinely reproducible ranking, not noise. Do
not expect these exact numbers, or the exact iteration counts where
noise stops dominating, on a different machine — the qualitative point
is that *how much a single run's ranking can be trusted* depends on
measurement length, not that the ranking itself must flip.

**Interpretation:** the fixed, one-time costs paid once per run — thread
start/join, branch-predictor and cache warm-up, frequency-scaling
ramp-up — are a small, roughly constant absolute cost regardless of
iteration count, so they (and ordinary scheduling noise alongside them)
dominate a *short* run's measurement and shrink to irrelevance in a
*long* one, exactly `T_fixed`'s shrinking share of `T(N)` as `N` grows.
That is why the short-length row is noisy enough that `int` and `mixed`
are statistically indistinguishable, while the two longer-length rows
agree with each other on a small, stable, explicable gap: `mixed`
doing strictly more work per op (the unit caveat above), now visible
without being swamped by fixed-cost noise. If your own run's numbers
look different in the details, check whether the shortest length's
spread is still comparable to its own gap between medians — that
relationship, not any specific number, is what should hold on any
machine.

**Cleanup:** none.

**Fallback path:** the script uses `python3` only for JSON parsing and
computing the median/min/max. If `python3` isn't available, run the
`cyclelab compute --iterations=... --op=...` commands directly, several
times per configuration, and compare `results.throughput_ops_per_s`
across repetitions by eye — `cyclelab` itself has no dependency beyond a
C11 toolchain and pthreads.

## Common Misconceptions

### *"A program has one true performance number." (M21)*

**Why it's wrong:** "Faster" is only meaningful for a stated workload,
input, and metric; the library story shows a change helping one input
size and hurting another, and the Guided Lab shows the narrower but
equally important cousin of that same mistake: a ranking measured from
a single short run can be pure noise, indistinguishable from a ranking
measured from a single long run that happens to agree with it by
chance — "faster" needs a stated measurement length as much as a stated
input size or metric.

**Correct intuition:** Run the same change across multiple input sizes
or metrics (and, per Chapter 4, multiple repetitions at each) and check
whether the ranking of "which version is faster" stays the same — if it
flips, or if it's not even reproducible run to run, there was never a
single number to begin with, only several questions that happened to
share a name.

**Analogy:** Asking "which car is faster" without saying faster at what
is like asking which vehicle wins a race without naming the course — a
sports car wins a straight sprint, a truck wins hauling cargo up a hill,
and neither answer is wrong, because they were never answering the same
question.

## Practical Implications

Before benchmarking anything, write down the workload model first: what
operation, at what rate or size, judged against what service level. A
benchmark result gathered without that step is a number in search of a
question. When someone reports "we made X faster," the next question is
always "faster for what workload, judged by what metric" — not because
the claim is being doubted, but because it's genuinely incomplete without
an answer.

A benchmark is a controlled scientific experiment, and this chapter's
step — writing down the operation and metric before measuring anything —
is that experiment's first, non-negotiable requirement: stating what
you're actually testing. Chapter 4 builds out the rest of that same
experiment (repetition, control, uncertainty); this chapter is where the
question itself gets defined precisely enough to be worth running at
all.

## Key Takeaway

**A performance result is meaningful only when the workload, metric, and
operating conditions are explicit.**

## What to Remember

- "Faster" is not a property of code alone; it's a property of code
  evaluated against a stated workload and metric.
- Completion time, response time (often at a tail percentile), and
  capacity under a fixed budget are three legitimate, different goals.
- A change that trades fixed overhead for marginal cost (or vice versa)
  will affect small and large workloads in opposite directions.
- Cost per unit of work matters even when elapsed time doesn't change.
- A benchmark number without a stated input size, measurement length,
  and workload model cannot be safely generalized.
- Two teams can both be right about the same change having opposite
  effects, if they're measuring different operations.
- A short measurement's ranking can be pure noise; trust a ranking only
  once its gap between configurations clearly exceeds the spread across
  repeated runs of each one (Chapter 4).

## Further Reading

- Brendan Gregg, *Systems Performance: Enterprise and the Cloud*, 2nd ed.
  — the workload characterization material in the early chapters is the
  direct ancestor of this chapter's "operation / workload model" framing.
  See `references/bibliography.md`.

## The Next Obvious Question

How do latency, throughput, utilization, and saturation differ?
