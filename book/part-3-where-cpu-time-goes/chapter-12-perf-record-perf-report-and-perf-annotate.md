# `perf record`, `perf report`, and `perf annotate`

**Part:** Part III — Where the CPU Time Goes
**Concept level:** 3
**Prerequisites:** counting, sampling, tracing (Chapter 11)
**New concepts:** sample, period, frequency, overhead percentage, inclusive versus self cost, call graph, annotation, source mapping

## Opening Question

Which functions and code paths consume CPU time?

## Incident or Real-World Story

An engineer profiles a slow request handler and gets back a report
listing dozens of functions, each with a percentage next to it. The
top entry, at 38%, is a small utility function called from a dozen
different places in the codebase. The engineer optimizes that function
directly — inlines a check, removes an allocation — and reruns the
profile. The function's own percentage drops to 12%. Total request time
barely moves.

The mistake becomes clear once the profile is read more carefully:
that function's 38% was almost entirely *inclusive* cost, accumulated
from the many different, unrelated call sites that happened to funnel
through it — not 38% of time spent *inside* the function's own code. Its
actual *self* cost, the time spent executing its own instructions rather
than the functions it called into, was closer to 3%. The engineer had
correctly identified the function with the largest number next to it,
and incorrectly assumed that number meant what they needed it to mean.

## Predict Before Measuring

Before reading further: if function `A` calls functions `B` and `C`, and
`B` and `C` are each individually expensive, what would you expect a
profiler to report as `A`'s own cost — close to zero, close to the sum
of `B` and `C`'s cost, or something else entirely? Which of those numbers
would actually tell you whether `A`'s own code (not the functions it
calls) is worth optimizing?

## Worked Example

This chapter's opening story is the worked example directly: a function
positioned as a common utility, called from many places, will
accumulate a large *inclusive* cost purely from being on the call path
of a lot of otherwise-unrelated work — that's a property of where it
sits in the call graph, not of how expensive its own code is. A leaf
function doing real, repeated arithmetic (no further calls out) will
show *self* cost close to its *inclusive* cost, because there's nowhere
else for the time to have gone. Distinguishing these two shapes — "high
self cost" versus "high inclusive cost, low self cost" — is the single
most consequential reading skill a CPU profile requires, and it's easy
to get backwards under time pressure, exactly as in the story.

## Core Intuition

A **sample** is one snapshot of what's currently executing, taken by a
profiler; the **period** is how much time (or how many events) elapse
between samples, and its inverse, **frequency**, is how many samples are
taken per second. **Overhead percentage** is a sample count expressed as
a share of the total samples taken, which is a statistical estimate of
time share, not a direct time measurement — precise only to the extent
the sampling frequency and run length made it so. **Inclusive cost** is
the share of samples where a function appears *anywhere* in the active
call stack — including time spent in everything it called. **Self
cost** is the share of samples where a function is the *innermost*
(currently executing) frame — time spent in its own code specifically.
A **call graph** is the tree of caller/callee relationships a profiler
reconstructs from sampled stacks. **Annotation** maps sampled cost back
onto source lines within one function; **source mapping** is the
underlying mechanism (debug information connecting compiled addresses
back to source file and line) that makes annotation possible at all.

## Technical Explanation

The core workflow this chapter teaches — capture samples, read an
aggregate report, then zoom into one function's source-line detail — has
three stages with three different questions:

1. **Capture** (`perf record` on Linux) collects samples at some
   frequency, with or without call-stack unwinding, over a run.
2. **Report** (`perf report`) aggregates those samples into a ranked
   list of functions, by default sorted by inclusive cost with self cost
   shown alongside — the view that requires reading self versus
   inclusive correctly, as this chapter's story shows.
3. **Annotate** (`perf annotate`) zooms into one function specifically,
   mapping its samples onto source lines (or disassembly, when source
   mapping isn't available — Chapter 13's subject) to show *which part*
   of that function's own code accounts for its self cost.

Overhead percentage deserves its own caution: it's a sampling estimate,
built from however many samples landed during the run, not a stopwatch
measurement of elapsed time in that function. A function sampled at 10%
over a short run with few total samples carries much more statistical
noise than the same 10% over a long run with many samples — the same
Chapter 4 concern (a single measurement's reliability depends on how
much evidence it's built from) applies to a profiler's percentages
exactly as it applies to a benchmark's average.

## Tool View

- What is measured: this chapter's portable lab uses macOS `sample` as a
  direct analog to the capture-report-annotate workflow: its "Sort by
  top of stack" output is a self-cost ranking (`perf report`'s
  equivalent), and its per-sample source-line references (when built
  with debug symbols, as `cyclelab` always is) are `perf annotate`'s
  equivalent.
- What is not measured: neither tool measures *why* a hot line is
  hot — a profiler locates cost; explaining it needs the mental models
  Part II already built (dependencies, stalls, mispredictions) or the
  memory model Part IV builds next.
- Required permissions: none for `sample` on this machine; `perf record`
  on Linux needs the same counter-access permissions as `perf stat`
  (Chapter 10).
- Likely overhead: sampling at a reasonable frequency (macOS `sample`'s
  default; Linux `perf record`'s common `-F 99`) is normally small
  relative to the workload; very high frequencies or full stack capture
  on every sample raise it, per Chapter 11's overhead discussion.
- Portability: on Linux, the equivalent workflow is:

  ```bash
  perf record -g -- ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=1 --chains=1
  perf report          # ranked functions, inclusive + self cost
  perf annotate compute_worker   # source/disassembly-level detail for one function
  ```

  **Documented, not tested** on this book's macOS reference machine.
- Common failure mode: reading a ranked report's top entry as
  automatically the best optimization target without checking whether
  its cost is self or inclusive — this chapter's entire story.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).
Requires macOS's built-in `sample`(1).

**Command:**

```bash
./labs/scripts/ch12_profile_hot_path.sh
```

This profiles `cyclelab compute --chains=1` (a single dependency chain,
so `compute_worker` is unambiguously both the hottest and the only
substantial self-cost frame) and prints `sample`'s self-cost ranking
plus a per-source-line breakdown inside `compute_worker`.

**Expected qualitative result:** `compute_worker` should dominate the
self-cost ranking (alongside `__ulock_wait`, which is the *other*
thread's time spent blocked in `pthread_join` — off-CPU time that
happens to still get sampled as "currently scheduled," a nuance Chapter
29 returns to). The per-line breakdown should concentrate unevenly
across `compute_worker`'s lines, not spread evenly. One example run on
the reference machine for this book showed:

```text
compute.c:0      804   (address without a clean line mapping -- see note below)
compute.c:105    436
compute.c:90     146
compute.c:89      94
compute.c:88      69
compute.c:114     24
compute.c:119      5
```

**Interpretation:** the concentration itself is the point — some lines
inside one "hot function" cost far more than others, exactly what
`perf annotate` is for on Linux. The `compute.c:0` entries are a real,
honest artifact worth understanding rather than ignoring: at `-O2`, some
sampled addresses land in code the compiler has restructured enough
(inlining, instruction reordering) that a single, clean source-line
mapping no longer exists for them — a preview of Chapter 13's subject.
Do not expect these exact line numbers or proportions on a different
compiler or architecture.

**Fallback path:** if `sample` isn't available, the self-versus-inclusive
distinction can still be reasoned about directly from source: pick any
function called from multiple places in a codebase you have access to,
and ask whether its own body does real work (self cost likely matters)
or mostly delegates to other functions (inclusive cost will dominate,
self cost will be small) — the judgment this chapter's lab makes
empirical, reasoned through by hand.

**Cleanup:** none (the script leaves `/tmp/ch12_sample.txt` for further
inspection, on purpose).

## Common Misconceptions

### *"Sampling profiles show all latency." (M08)*

**Why it's wrong:** A CPU profile like this chapter's only shows
*on-CPU* execution — Chapter 11's sampling model, which needs something
running to sample. Time spent blocked, waiting, or sleeping never shows
up as a hot function here, even when it dominates a request's actual
latency (Chapter 1's whole opening problem).

**Correct intuition:** This chapter's lab's `__ulock_wait` entry is
exactly off-CPU time (`pthread_join` blocking) that a naive reading
could mistake for "hot code," when it's actually the *absence* of
running code — full treatment of what off-CPU sampling requires is
Chapter 29.

**Analogy:** A dashboard camera only records what's happening while the
car is moving — if the car sits idling at a closed rail crossing for ten
minutes, that footage shows nothing "hot" at all, even though it's
exactly where the trip's time went.

### *"The function with the highest cost in a profile is always the best optimization target."*

**Why it's wrong:** A high *inclusive* cost can come entirely from a
function's position in the call graph (called from many places) rather
than from its own code being expensive — this chapter's opening story
directly.

**Correct intuition:** Check self cost, not just inclusive cost, before
deciding where to spend optimization effort — a function with high
inclusive cost but low self cost points at *its callees*, not at
itself.

**Analogy:** A company's CEO "touches" nearly every dollar that flows
through the org chart on its way to being spent, but that doesn't mean
the CEO is personally where the money goes — you have to look at who's
actually spending it, not just who it passed through.

### *"Kernel frames in a profile are irrelevant to application performance."*

**Why it's wrong:** Time an application's own code causes to be spent
in the kernel (a syscall, a page fault, a scheduling decision) is still
time that request or workload waited for, even though the code
executing during it isn't the application's own — this chapter's
`__ulock_wait` frame is exactly a kernel-side frame that matters to
interpreting the profile correctly, not one to filter out reflexively.

**Analogy:** Time your delivery driver spends waiting at a warehouse
loading dock still counts against your delivery's total time, even
though the warehouse isn't part of your own company — filtering out
"someone else's building" from the timeline would just hide where the
delay actually happened.

## Practical Implications

Before optimizing whatever function sits at the top of a profile,
check whether its cost is self or inclusive. A high inclusive, low self
cost function is a signpost pointing at its callees, not a target in its
own right; optimizing it directly (as in this chapter's story) can look
like progress on paper — the function's own percentage really does drop —
while doing almost nothing for the metric that actually matters.

## Key Takeaway

**A CPU profile shows where sampled on-CPU execution accumulated, not
why the code was slow or what happened while it was off CPU.**

## What to Remember

- Overhead percentage is a statistical estimate from sampling, not a
  stopwatch measurement — treat it with Chapter 4's variance discipline.
- Inclusive cost includes everything a function called into; self cost
  is only its own code — conflating the two misdirects optimization
  effort, as this chapter's incident shows directly.
- A function can have high inclusive cost purely from its position in
  the call graph, with almost no self cost of its own.
- Annotation maps a function's sampled cost onto its source lines, built
  on the same debug information (source mapping) that makes readable
  disassembly possible (Chapter 6).
- A CPU profile only shows on-CPU time; blocked, waiting, or sleeping
  time is invisible to it by construction (M08) — full off-CPU treatment
  is Chapter 29.
- Kernel-side frames in a profile represent real time an application's
  own behavior caused, not noise to filter out.

## Further Reading

- Linux perf manual pages: <https://man7.org/linux/man-pages/man1/perf.1.html>
- `sample`(1) manual page (macOS): `man sample`.

## The Next Obvious Question

Why are call stacks sometimes missing or wrong?
