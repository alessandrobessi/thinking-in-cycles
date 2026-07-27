# Cache Coherence and False Sharing

**Part:** Part IV — Why Memory Changes Everything
**Concept level:** 4
**Prerequisites:** cache line, working set, reuse (Chapters 16-17)
**New concepts:** shared cache line, ownership, invalidation, coherence traffic, true sharing, false sharing, HITM intuition

## Opening Question

How can independent threads slow each other through cache coherence?

## Incident or Real-World Story

A team parallelizes a counting task by giving each worker thread its own
dedicated counter — no shared state, no lock, nothing in the source code
that would suggest any thread depends on any other. Scaling tests show
a disappointing result: doubling the thread count barely improves
throughput, and past a certain point, adding threads makes things
*worse*. Every code review confirms there's no logical sharing — each
thread reads and writes only its own counter, declared as one element of
a small array allocated together for convenience.

That last detail is the whole problem. The counters, being small and
declared together, all landed inside the same 64-byte cache line. From
the source code's perspective, the threads own separate variables. From
the cache-coherence protocol's perspective — which operates on whole
cache lines, with no visibility into how a program's source code
divides one up — every thread's write to its own counter invalidates
that entire line for every other thread sharing it, forcing them to
refetch it before their next access. The threads never touched each
other's data. They spent enormous effort fighting over a cache line
anyway.

## Predict Before Measuring

Before reading further: if N threads each increment their own
independent counter with no locking, and the counters happen to sit in
the same cache line, do you expect throughput to scale with thread
count the same way it would if the counters were on separate cache
lines — and if not, do you expect the gap to be roughly constant or to
grow as more threads are added?

## Worked Example

This chapter's opening story is also its canonical example: per-thread
counters placed adjacently in one small array or struct. Whether this
causes a problem is entirely a layout question, invisible at the level
of "which variable does this thread touch" — two counters 8 bytes apart
in a packed array very likely share a line; the same two counters, each
padded out to consume a full cache line on their own, cannot.

## Core Intuition

A **shared cache line** is a line more than one core has a copy of. Each
cache line has an **ownership** state under the coherence protocol
(commonly some variant of MESI): a core writing to a line it doesn't
exclusively own must first **invalidate** every other core's copy,
forcing them to fetch fresh data before their next access. This
back-and-forth is **coherence traffic** — real bus/interconnect activity
that costs real time, invisible to source code. **True sharing** is
coherence traffic from genuinely shared data (multiple threads actually
reading and writing the same logical variable) — real contention with a
real fix (a different algorithm, a lock, an atomic). **False sharing**
is coherence traffic from *unrelated* variables that merely happen to
share a cache line — no logical contention exists, so the fix is purely
about memory layout, not algorithm. The canonical picture: two people
repeatedly erasing and rewriting separate fields on one shared
whiteboard, each getting in the other's way purely because they're
working on the same physical surface, not because either one cares what
the other wrote — the granularity that matters is the whiteboard (the
cache line), not the field (the variable). **HITM intuition** (informally: a
read or write that "hits" a line another core has modified) is the
underlying hardware event this traffic shows up as when a counter tool
can see it directly (Chapter 20).

## Technical Explanation

The coherence protocol operates at cache-line granularity because
that's the unit hardware actually tracks — it has no way to know, and
no reason to care, that a program's source code considers two 8-byte
regions of the same line to be unrelated variables. Any write by any
core to any part of a shared line triggers the same invalidation
machinery, whether that write is "real" contention or not. This is
exactly why false sharing is purely a layout problem: padding each
hot, independently-written variable out to its own cache line removes
the possibility of triggering another thread's invalidation, without
changing what the program computes at all.

The mandatory cautions worth stating directly: padding increases memory
footprint, sometimes substantially, since a single 8-byte counter now
consumes a full 64-byte line — a real cost, not a free fix. Not every
line shared between threads is false sharing; some sharing is true and
needs an actual synchronization fix, not padding. And false sharing's
severity is highly thread-count- and topology-dependent: this chapter's
lab shows a modest gap at low thread counts that grows substantially at
higher ones, and a different core topology (more sockets, different
cache-coherence interconnect) can show a very different shape entirely.

## Tool View

- What is measured: throughput scaling across thread counts, for
  otherwise-identical code differing only in counter memory layout,
  using `cyclelab false-sharing --padding=packed|padded`.
- What is not measured: coherence events directly — a HITM-style counter
  would show cache-to-cache transfers explicitly; this chapter's
  portable lab infers the effect from its throughput consequence
  instead. Chapter 20 covers `perf c2c`, the direct Linux tool for this.
- Required permissions: none.
- Likely overhead: negligible for the measurement itself.
- Portability: works anywhere `cyclelab` runs; the *magnitude* of any
  false-sharing penalty is architecture-, core-count-, and
  topology-specific — treat this chapter's numbers as one machine's
  result, not a universal constant.
- Common failure mode: running this comparison for too short a duration
  and mistaking run-to-run noise for a real signal — this chapter's own
  script defaults to a full second per run specifically because shorter
  runs showed visibly noisier, sometimes non-monotonic results during
  testing for this book.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch18_false_sharing.sh
```

This runs `cyclelab false-sharing` at thread counts 1, 2, 4, 8, and 10,
for both `--padding=packed` and `--padding=padded`, tabulating
throughput.

**Expected qualitative result:** at `--threads=1`, packed and padded
should be roughly equal (nothing to share with only one thread). As
thread count rises, padded should increasingly outperform packed. One
example run on the reference machine for this book (Apple M4, macOS,
arm64, 10 logical CPUs) showed:

```text
padding  threads  throughput_incr_s
packed   1        1,095,871,075
padded   1        1,101,491,637
packed   8        4,323,376,492
padded   8        5,373,728,288
packed   10       5,064,709,887
padded   10       6,231,229,929
```

At 1 thread, packed and padded were within 1% of each other. At 8
threads, padded led by roughly 24%.

**Interpretation:** the gap's exact size is specific to this machine —
different core counts, cache hierarchies, and coherence interconnects
change how expensive false sharing is, sometimes substantially. The
qualitative shape (roughly equal at 1 thread, a growing gap as thread
count increases) is what to look for, not this exact percentage.

**Fallback path:** if `python3` isn't available, run the ten
`cyclelab false-sharing --padding=... --threads=...` commands directly
and read `results.throughput_increments_per_s` from each run's raw
JSON.

**Cleanup:** none.

## Common Misconceptions

### *"Padding every shared structure is a safe, free optimization." (M34)*

**Why it's wrong:** Padding increases memory footprint — sometimes
drastically, since a single small field padded to a cache line can
inflate a structure's size by an order of magnitude — and applying it to
data that isn't actually experiencing false sharing wastes memory and
can hurt locality elsewhere for no benefit at all.

**Correct intuition:** Measure scaling with and without padding, as in
this chapter's lab, before applying it — don't pad reflexively.

**Analogy:** Giving every guest at a dinner party their own private
table "just in case" avoids any chance of them bumping elbows, but it
also means renting a much bigger hall for no reason if most guests were
never going to sit near each other in the first place.

## Practical Implications

Before parallelizing a workload with per-thread state, check how that
state is laid out in memory, not just whether each thread's logic looks
independent. Per-thread counters, accumulators, or flags declared
together in one array or struct are exactly the shape that invites false
sharing — a purely mechanical fix (padding, or restructuring to give
each thread's data its own cache line) that requires no algorithmic
change at all, but only pay for it where a measurement like this
chapter's actually shows a gap.

## Key Takeaway

**Threads can contend through the cache-coherence protocol even when
the source code says they own different variables.**

## What to Remember

- Cache coherence operates at cache-line granularity, with no awareness
  of how a program's source code subdivides that line into variables.
- True sharing is real contention over genuinely shared data; false
  sharing is coherence traffic from unrelated variables that merely
  share a line — the fixes for each are completely different.
- Padding removes the possibility of false sharing by giving each hot
  variable its own cache line, at the cost of increased memory
  footprint.
- Not every line shared between threads is false sharing — check before
  assuming.
- False-sharing severity depends heavily on thread count and hardware
  topology; a result on one machine does not transfer to another.
- Short benchmark durations can make this specific comparison noisy;
  Chapter 4's repetition discipline applies directly here.

## Further Reading

- Linux false-sharing documentation: <https://docs.kernel.org/kernel-hacking/false-sharing.html>

## The Next Obvious Question

How do we tell whether a workload is limited by memory bandwidth?
