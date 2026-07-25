# Chapter 11 — Counting, Sampling, and Tracing

**Part:** Part III — Where the CPU Time Goes
**Concept level:** 3 (Level 2 concepts from Part II assumed)
**Prerequisites:** cycle, instruction, IPC, stall, PMU, counter (used informally, Chapter 10)
**New concepts:** counting, sampling, tracing

## Opening Question

When should we count, sample, or trace?

## Incident or Real-World Story

A service's dashboard shows CPU time is dominated by one code path —
roughly 40% of every core's busy time, day in and day out, according to
a sampling profiler run during business hours. An engineer optimizes
that path, ships the change, and CPU usage on the dashboard drops
exactly as predicted. Users, however, keep reporting the same complaint
they had before: every so often, a request takes half a second instead
of the usual few milliseconds. The sampling profiler that found the
40% hot path never saw this problem at all, because it wasn't a CPU
problem in the sense sampling measures — it was one rare, specific event
happening a few times an hour, invisible in an aggregate view built from
periodic snapshots.

Finding it took a different kind of tool entirely: one that recorded
*every* occurrence of a specific event (a particular lock acquisition, a
particular downstream call) with a timestamp, rather than periodically
asking "what's running right now." That reconstructed sequence showed
the rare event clearly — a specific retry path that occasionally waited
on a saturated connection pool — something no amount of staring at the
40% hot path would ever have revealed, because the hot path and the tail
latency problem were, in this case, almost entirely unrelated.

## Predict Before Measuring

Before reading further: if a rare, expensive event happens 3 times per
hour and lasts 500ms each time, and a sampling profiler takes a snapshot
every 10 milliseconds, roughly how many of those snapshots do you expect
to land during one of those 500ms windows over a full hour of sampling?
Does that number strike you as enough to reliably characterize the
event, or not? Hold that estimate for the Core Intuition section.

## Worked Example

This chapter's opening story is itself the canonical worked example,
worth restating in its own vocabulary: a service spends roughly 40% of
CPU time in one code path, continuously — and separately, a rare event
occurring a few times an hour adds 500ms to whatever request triggers
it. A sampling profiler, built on periodic snapshots, is well-suited to
finding the first kind of problem (a large, steady share of *time*
naturally accumulates a large *share of snapshots*) and poorly suited to
the second (a rare event is, definitionally, rarely caught mid-snapshot,
no matter how it affects tail latency). A tracing tool, built on
recording specific events as they happen rather than periodically
sampling, is the reverse: it can catch every occurrence of a rare,
specific event by design, at the cost of needing to already know
*which* event to watch for.

## Core Intuition

**Counting** answers *how many*: a running tally of a specific event
(instructions retired, requests served, cache misses), with no
information about when in the run they happened or what code triggered
each one — the smallest overhead, the least detail. **Sampling** answers
*where execution tends to be*: periodically checking what's currently
running (or which call stack is active) and building up a statistical
picture from many such snapshots — the canonical picture is periodically
photographing a factory floor: a photo taken every few seconds tells you
a great deal about which stations are usually busy and says almost
nothing about a single rare, brief event between photos. Sampling is
cheap per snapshot, but only reliable for things that occupy a large
enough share of time to show up across many snapshots; the Predict
Before Measuring estimate above is usually small (a few snapshots out
of thousands, for a rare, short event) — plausible to catch by luck, not
something to rely on.
**Tracing** answers *what happened, in sequence*: recording specific,
individually meaningful events as they occur, with timestamps and
context — capable of catching every occurrence of even a very rare
event, at the cost of needing to instrument specifically for it and of
higher overhead per event recorded.

## Technical Explanation

The three models aren't ranked by sophistication — each is the right
answer to a different question, and each has a real cost structure
worth naming explicitly:

- Counting overhead is normally negligible: incrementing a counter (in
  hardware, via the PMU, or in software) costs very little regardless of
  how often it happens, and the output is a handful of numbers, not a
  growing log.
- Sampling overhead grows with **sample frequency** (how often a
  snapshot is taken) and **stack capture cost** (walking and recording a
  full call stack is more expensive than just noting the current
  instruction pointer) — a tradeoff between statistical resolution and
  the very thing being measured (Chapter 4's perturbation caution,
  applied directly to the measurement tool itself).
- Tracing overhead grows with **event rate** and **payload size**: a
  rarely-firing tracepoint with a small payload is cheap; instrumenting
  a high-frequency event (every function call, every allocation) with a
  large captured payload (full arguments, full stack) can measurably
  slow down the very thing being traced, and produces **output volume**
  that itself becomes a problem to store and analyze.

Choosing among the three is a direct consequence of Chapter 5's
investigation loop: the hypothesis determines which question you're
actually asking, and the question determines which of these three models
can even answer it.

## Tool View

- What is measured: this chapter's portable lab compares a **counting**
  view (`cyclelab`'s own JSON summary: total iterations, throughput) and
  a **sampling** view (macOS `sample`, via `labs/scripts/foldstacks.py`)
  of the exact same run.
- What is not measured: the portable lab has no **tracing** component —
  event-level tracing on this book's macOS reference machine
  (`dtrace`, `fs_usage`) requires root, which this sandboxed environment
  doesn't have; on Linux, `perf trace` or a tracepoint-based `bpftrace`
  one-liner is the natural equivalent, previewed below and covered in
  full starting Chapter 26.
- Required permissions: counting and sampling, none, on this machine;
  tracing, root or `CAP_SYS_ADMIN`-equivalent on Linux.
- Likely overhead: counting negligible; sampling small at `sample`'s
  default rate; tracing varies enormously by event rate, as above.
- Portability: `sample` is macOS-specific. On Linux, the closest
  equivalent commands are:

  ```bash
  # counting
  perf stat -- ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=1

  # sampling
  perf record -F 99 -g -- ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=1
  perf report

  # tracing (one specific event, not a general sweep)
  perf trace -e sched_switch -- ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=1
  ```

  **These three Linux commands are documented, not tested** on this
  book's reference machine.
- Common failure mode: reaching for a trace when a sample would answer
  the question more cheaply (or vice versa) — matching the wrong
  observation model to the question wastes both effort and, in
  tracing's case, real overhead on the system being measured.

## Guided Lab

**Portability:** portable (counting + sampling); the tracing preview
above is **privileged** / Linux-only.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).
Requires macOS's built-in `sample`(1) (present by default; no install
needed) and `python3`.

**Command:**

```bash
# 1. Counting view: cyclelab's own summary.
./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=1 --chains=1 --quiet

# 2. Sampling view: capture + fold a real profile of the same workload shape.
./labs/scripts/capture_sample_profile.sh /tmp/ch11.folded 2 -- \
  compute --duration=3 --threads=1 --chains=1 --quiet --output=/dev/null
cat /tmp/ch11.folded
```

**Expected qualitative result:** the counting view answers "how much
work got done and how fast" — a single throughput number and an
iteration count, nothing about *where* time went. The sampling view
answers a different question: `/tmp/ch11.folded` should show almost all
samples landing in `compute_worker` (the hot loop), with only a handful
in `timing_now_seconds` and its callees (the periodic clock check) — the
*shape* of where execution spent its time, with no throughput number in
sight.

**Interpretation:** neither view is more "correct" than the other — they
answer different questions, exactly this chapter's point. If you wanted
to know whether a change made the workload faster, the counting view
(throughput) is what actually answers that. If you wanted to know which
function to optimize next, the sampling view is what answers that. Using
one to answer the other's question is the mismatch this chapter warns
against.

**Fallback path:** if `sample` isn't available (non-macOS, non-Linux, or
a locked-down environment), the counting half of this lab stands alone —
rerun the first command a few times at different `--chains` values
(Chapter 7/8's labs) and note that counting alone already tells you
*that* something changed, just never *where in the code* it changed.

**Cleanup:** none.

## Common Misconceptions

There is no blueprint-seeded misconception registry entry specific to
this chapter's three-model framing; a new one is worth naming directly:
**"A sampling profiler will eventually catch any performance problem
if you just run it long enough."** This is wrong because a rare event's
chance of being caught by a periodic sample depends on its *duration*
relative to the sampling interval, not on how long the profiler runs in
total — running ten times longer roughly gives ten times as many
*chances*, but a very short, rare event can still remain effectively
invisible in aggregate sampled output even after a long run, while a
tracing tool built to watch for that specific event catches it every
time by construction. The evidence that distinguishes the two: compare
a sampling profile's coverage of a known rare event against a trace of
that same event over an identical time window — the trace will show
every occurrence; the sample will show, at best, a probabilistic subset.

## Practical Implications

Before choosing a tool, state the question in one of these three
shapes — "how many/how much" (counting), "where does time tend to go"
(sampling), or "what exact sequence of events happened" (tracing) — and
let that shape pick the tool, rather than defaulting to whichever tool
is most familiar. A rare-event tail-latency investigation reached for
with only a sampling profiler, as in this chapter's story, can waste
real time looking in the wrong kind of place.

## Key Takeaway

**Choose the observation model that matches the question: counts
summarize, samples locate, and traces reconstruct.**

## What to Remember

- Counting answers "how many/how much," with the lowest overhead and
  the least detail about where or when.
- Sampling answers "where execution tends to be," built from periodic
  snapshots — reliable for things that occupy a large share of time,
  unreliable for rare, short events.
- Tracing answers "what happened, in sequence," catching every
  occurrence of a specifically instrumented event, at higher overhead
  per event and requiring you to already know what to watch for.
- Overhead scales differently for each: counting is nearly free,
  sampling scales with frequency and stack-capture cost, tracing scales
  with event rate and payload size.
- A rare event that matters for tail latency can be completely invisible
  to a sampling profiler no matter how long it runs.
- Matching the tool to the actual shape of the question is a direct
  extension of Chapter 5's investigation loop, not a separate skill.

## Further Reading

- Linux perf documentation: <https://docs.kernel.org/admin-guide/perf/index.html>
- `sample`(1) and `dtrace`(1) manual pages (macOS) — `man sample`,
  `man dtrace`.

## The Next Obvious Question

Which functions and code paths consume CPU time?
