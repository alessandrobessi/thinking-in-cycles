# Chapter 2 — Performance Is a Question, Not a Number

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
that prediction — the Guided Lab measures something structurally similar.

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

The library story generalizes into a rule: any change that shifts *fixed,
per-operation overhead* against *marginal, per-unit-of-work cost* will
affect small-input and large-input workloads in opposite directions. A
large up-front allocation is fixed overhead, paid once; it is invisible
when amortized over millions of records and dominant when there's only
one record to amortize it over. This isn't specific to buffers — the same
shape shows up in thread-pool warm-up, connection pooling, JIT
compilation, cache warming, and batch-size tuning of every kind. None of
these are bugs in the technique. They are properties of *which* workload
model you evaluate them against.

This is also why **service level** has to be stated, not assumed. "Keep
p99 under 100ms" and "finish the batch by 6 a.m." are both legitimate
service levels, but a change evaluated against the wrong one will report
a false verdict — improved by one standard, regressed by the other,
simultaneously true.

## Tool View

No new measurement tool is needed yet — the tool from Chapter 1 (compare
wall time / user time / system time, or here, throughput at varying input
size) is enough, applied more deliberately:

- What is measured: a metric (here, throughput) at more than one input
  size, for more than one configuration, so a comparison can be made
  *within* each size rather than pooled across sizes.
- What is not measured: which size is "the real one" — that's a decision
  about the workload model, not something a measurement can answer for
  you.
- Required permissions: none.
- Likely overhead: negligible.
- Portability: works anywhere `cyclelab` runs.
- Common failure mode: reporting a single benchmark number ("20% faster")
  without saying at what input size it was measured, then being surprised
  when it doesn't generalize.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch2_size_sweep.sh
```

This runs `cyclelab compute` at three fixed iteration counts (a small,
medium, and large "input size") with two operation mixes (`--op=int` and
`--op=mixed`), and tabulates throughput for each combination.

**Expected qualitative result:** the *ranking* of which configuration has
higher throughput should not have to be the same at the smallest size as
at the largest size. One example run on the reference machine for this
book (Apple M4, macOS, arm64, `cyclelab` release build) showed exactly
that pattern:

```text
iterations   op       throughput_ops_s
200000       int      169,671,262
200000       mixed    245,587,110
5000000      int      313,828,869
5000000      mixed    309,549,605
100000000    int      321,417,451
100000000    mixed    250,123,107
```

At the smallest size, `mixed` outperformed `int`; at the largest size,
`int` outperformed `mixed`. Do not expect these exact numbers, or even
necessarily a crossover at the same sizes, on a different machine — the
qualitative point is that the ranking can flip at all, not the specific
sizes where it does.

**Interpretation:** if your run shows a similar reordering between the
smallest and largest size, you've reproduced the chapter's point directly:
per-run fixed costs (thread start/join, warm-up, the fixed cost of
`--op=mixed`'s extra branch inside the accumulator loop) matter
proportionally more at small iteration counts than large ones. If your
run doesn't show a clean reordering, that's still informative — it means
this particular pair of configurations isn't a good example of the effect
on your machine, which is a legitimate, useful negative result.

**Cleanup:** none.

**Fallback path:** the script uses `python3` only to format `cyclelab`'s
JSON output into a table. If `python3` isn't available, run the six
`cyclelab compute --iterations=... --op=...` commands from the script
directly and read `results.duration_actual_s` and
`results.throughput_ops_per_s` from the raw JSON by eye — `cyclelab`
itself has no dependency beyond a C11 toolchain and pthreads.

## Common Misconceptions

**M21 (proposed) — "A program has one true performance number."** This is
wrong because "faster" is only meaningful for a stated workload, input,
and metric; as the library story and the Guided Lab both show, the same
change can help one input size or metric and hurt another. The evidence
that distinguishes the two: run the same change across multiple input
sizes or metrics and check whether the ranking of "which version is
faster" stays the same — if it flips, there was never a single number to
begin with.

## Practical Implications

Before benchmarking anything, write down the workload model first: what
operation, at what rate or size, judged against what service level. A
benchmark result gathered without that step is a number in search of a
question. When someone reports "we made X faster," the next question is
always "faster for what workload, judged by what metric" — not because
the claim is being doubted, but because it's genuinely incomplete without
an answer.

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
- A benchmark number without a stated input size and workload model
  cannot be safely generalized.
- Two teams can both be right about the same change having opposite
  effects, if they're measuring different operations.

## Further Reading

- Brendan Gregg, *Systems Performance: Enterprise and the Cloud*, 2nd ed.
  — the workload characterization material in the early chapters is the
  direct ancestor of this chapter's "operation / workload model" framing.
  See `references/bibliography.md`.

## The Next Obvious Question

How do latency, throughput, utilization, and saturation differ?
