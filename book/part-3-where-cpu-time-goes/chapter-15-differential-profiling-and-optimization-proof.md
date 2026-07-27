# Differential Profiling and Optimization Proof

**Part:** Part III — Where the CPU Time Goes
**Concept level:** 3
**Prerequisites:** folded stack, frame width, CPU flame graph (Chapter 14); benchmarking hygiene, the investigation loop (Chapters 4-5)
**New concepts:** before/after profile, normalized workload, differential flame graph, `perf diff`, regression, bottleneck shift, total-work normalization

## Opening Question

How do we prove that an optimization changed the right thing?

## Incident or Real-World Story

An engineer profiles a slow function, finds it accounts for 30% of
samples, rewrites it, and reprofiles: the function's share drops to
almost nothing, exactly as hoped. They report success. A few days
later, someone checks the metric that actually matters — end-to-end
throughput of the service — and it barely moved. Digging further, the
saved time didn't disappear; it shifted. The optimized function used to
be the bottleneck; now a memory allocator and a lock it contends for are,
absorbing almost exactly the time the profiled function used to cost.
The optimization was real. Its effect on the metric anyone actually
cared about was not, because something else was waiting to become the
new limit the moment the old one moved.

The mistake wasn't in the optimization — it was in treating "this
function's profile share dropped" as equivalent to "the workload got
faster," without ever checking the second claim directly.

## Predict Before Measuring

Before reading further: if you profile a workload for exactly 3 seconds
before an optimization and exactly 3 seconds after, and the "after"
version completes roughly three times as much actual work in those 3
seconds, do you expect the two profiles' total sample counts to be
roughly equal, or roughly proportional to the work completed? Hold your
answer — this chapter's lab measures exactly this.

## Worked Example

This chapter's opening story is the canonical shape: an optimization
removes a large share of samples from one function, but total workload
throughput barely changes, because the saved time moved to a different
bottleneck (memory allocation, lock contention) rather than
disappearing. This is a **bottleneck shift** — first named operationally
in Chapter 5's investigation loop, now given its full profiling
vocabulary: a fixed bottleneck doesn't guarantee a faster workload, only
a different one, until whatever replaces it is checked too.

## Core Intuition

A **before/after profile** pair captures the same workload's shape both
before and after exactly one change — Chapter 5's "change one thing"
discipline, applied to profiling specifically. A **normalized workload**
is one held constant in whatever way makes the comparison meaningful:
same input, same duration *or* same completed work (a choice that must
be stated explicitly — see **total-work normalization** below). A
**differential flame graph** overlays two folded-stack captures and
colors each frame by whether its share grew, shrank, or stayed roughly
the same relative to the baseline — `perf diff` is the equivalent
aggregate (text, not visual) comparison on Linux. A **regression** is
the opposite of an improvement: a change that made some metric worse,
caught by exactly the same before/after discipline.

## Technical Explanation

**Total-work normalization is the single most important caution in this
chapter, and it's easy to get backwards.** Comparing two profiles
captured for the same *wall-clock duration* is not the same as comparing
them for the same *amount of completed work* — if the "after" version is
faster, it does more work in the same time window, and a profile
captured that way represents a different total amount of work than the
"before" one did, even though both ran for, say, exactly 3 seconds. This
doesn't make equal-duration profiles useless, but it means their
*shape* (proportions, which functions dominate) is what's comparable,
not their raw sample counts — and even the shape comparison assumes
sampling itself didn't change behavior between the runs, which the next
paragraph's real data shows can fail in a subtle way worth knowing
about.

State explicitly, every time, whether a before/after comparison is
normalized by time, by completed operations, or by some other unit —
this chapter's own lab, below, uses equal *sampling duration*, and its
Interpretation section explains exactly what that does and doesn't
prove as a direct consequence.

## Tool View

- What is measured: this chapter's lab combines a controlled, interleaved
  throughput benchmark (Chapter 4's discipline — the actual proof of
  improvement) with before/after profiles and a differential flame
  graph (explaining how the work's shape did or didn't change).
- What is not measured: the differential tool used here
  (`flamegraph_svg.py --diff-against`) matches frames by name at each
  tree position, the same approach `perf diff` and Brendan Gregg's
  differential flame graph tooling use — it cannot detect a change that
  moves work into a *renamed* function or restructures the call tree
  entirely without changing leaf function names.
- Required permissions: none for this chapter's lab.
- Likely overhead: sampling overhead only, as in prior Part III chapters.
- Portability: on Linux, `perf diff` is the standard aggregate
  (non-visual) equivalent:

  ```bash
  perf record -g -o before.data -- ./labs/cyclelab/bin/cyclelab compute --duration=3 --chains=1
  perf record -g -o after.data  -- ./labs/cyclelab/bin/cyclelab compute --duration=3 --chains=8
  perf diff before.data after.data
  ```

  **Documented, not tested** on this book's macOS reference machine.
- Common failure mode: treating a shrinking profile share as proof of
  improvement without checking the actual outcome metric — this
  chapter's opening story exactly.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).
Requires `python3`; the profiling half additionally requires macOS's
built-in `sample`(1) (the benchmark half works without it).

**Command:**

```bash
./labs/scripts/ch15_before_after.sh
```

This runs the full loop: an interleaved, controlled throughput
comparison between `--chains=1` ("baseline") and `--chains=8` ("after")
— the same real change Chapters 7-8 introduced — followed by before/after
profiles and a differential flame graph.

**Expected qualitative result:** the throughput comparison should show a
clear, consistent, large improvement for "after" over "baseline" across
every interleaved repetition — real proof, needing no profiler. The two
profiles' total sample counts should come out close to *equal* despite
"after" completing far more actual work in the same capture window. One
real run on the reference machine for this book showed:

```text
baseline total samples (main-thread path):  1579
after    total samples (main-thread path):  1579
```

Identical — even though "after" completed roughly three times the
iterations in that same 3-second capture, confirmed separately by the
interleaved throughput benchmark in step 1. The differential flame graph
showed mostly grey (unchanged shape) with a few small red frames from
tiny, incidental differences in the rarely-sampled timing-check branch —
noise, not signal, at that sample size (Chapter 4's variance caution,
directly).

**Interpretation:** this is total-work normalization made concrete —
equal sampling *duration* produced profiles of unequal *completed work*,
and their near-identical shape does not mean the two configurations did
the same amount of work; it means a shape-based view (sampling) is
nearly blind to a change that made existing code run faster without
changing which code ran. The throughput benchmark, a counting-based
view, is what actually proved the improvement — a direct callback to
Chapter 11's count-versus-sample distinction, now demonstrated instead
of just asserted.

**Fallback path:** if `sample` isn't available, step 1 (the interleaved
throughput benchmark) stands alone as the actual proof of improvement,
exactly as Chapters 4-5 already established — the profiling half only
explains mechanism, and this chapter's own results show that explanation
can legitimately come back "no visible shape change" for a real,
substantial improvement.

**Cleanup:** none.

## Common Misconceptions

### *"A microbenchmark improvement guarantees a production improvement." (M16, previewed)*

**Why it's wrong:** Chapter 30 treats this fully, but this chapter's
opening story is a narrower, related case worth flagging now: even
within a single controlled benchmark, a local profile improvement (one
function's share dropping) does not guarantee the metric that matters
improved, if a bottleneck shift absorbed the savings elsewhere.

**Correct intuition:** Measure the actual outcome metric before and
after, not just the shape of the profile — a smaller slice of a pie
chart doesn't tell you whether the pie itself got smaller.

**Analogy:** Losing weight in one specific place on your body doesn't
guarantee you lost weight overall — you have to step on the scale, not
just look in the mirror at the one spot you were focused on.

### *"An optimization is complete when the original hotspot shrinks." (M17, previewed)*

**Why it's wrong:** Also fully Chapter 30's territory, and also directly
this chapter's incident: the original hotspot did shrink; the workload
did not get meaningfully faster, because shrinking a hotspot is not the
goal — improving the outcome metric is, and a shrunk hotspot only
achieves that if nothing else was waiting to take its place.

**Correct intuition:** Confirm the workload's actual outcome metric
improved, and check where the new hotspot is — declaring victory at "the
old bottleneck is smaller" without that second check is exactly this
chapter's own incident.

**Analogy:** Fixing the slowest checkout line in a supermarket doesn't
speed up the store if a different line was always going to become the
new bottleneck the moment the first one sped up — the goal is shorter
overall wait, not a shorter specific line.

## Practical Implications

Before declaring an optimization successful, check the actual outcome
metric under controlled, interleaved conditions (Chapter 4), not just
whether a profile's shape changed in the direction you expected. If it
did improve, profile again to see whether the bottleneck moved — the
next investigation's starting point, not a separate project.

## Key Takeaway

**An optimization is proven by a better workload outcome under
controlled conditions, while profiles explain how the work changed.**

## What to Remember

- A before/after profile pair only explains a change's mechanism; the
  actual proof of improvement is the outcome metric, measured under
  Chapter 4's controlled, interleaved discipline.
- A bottleneck shift means saved time moved elsewhere, not that it
  disappeared — always worth checking after a real improvement, not just
  after a disappointing one.
- Comparing profiles by equal wall-clock duration is not the same as
  comparing them by equal completed work — state explicitly which
  normalization a comparison uses.
- A differential flame graph (or `perf diff` on Linux) colors frames by
  change relative to a baseline — red/blue there is the one place color
  does carry magnitude, unlike a normal CPU flame graph (Chapter 14).
- A real, substantial improvement can show almost no visible shape
  change in an equal-duration differential profile, if it made existing
  code faster rather than changing which code runs — a legitimate
  result, not a failed measurement.

## Further Reading

- Linux perf manual pages (`perf diff`): <https://man7.org/linux/man-pages/man1/perf.1.html>
- Brendan Gregg's differential flame graphs writeup (search "differential
  flame graphs brendan gregg") — the origin of the red/blue convention
  this chapter's tooling follows.

## The Next Obvious Question

Why can memory access dominate code that performs little computation?
