# Chapter 19 — Memory Bandwidth and the Roofline Intuition

**Part:** Part IV — Why Memory Changes Everything
**Concept level:** 4
**Prerequisites:** cache line, latency, locality (Chapter 16); coherence traffic (Chapter 18)
**New concepts:** bytes transferred, sustained bandwidth, peak bandwidth, arithmetic intensity, memory-level parallelism, bandwidth saturation, roofline intuition

## Opening Question

How do we tell whether a workload is limited by memory bandwidth?

## Incident or Real-World Story

A team adds more worker threads to a data-processing pipeline that
streams through large in-memory buffers, expecting throughput to keep
climbing the way it did the first few times they tried this. It does —
until it doesn't. Past a certain thread count, adding more workers stops
helping at all; CPU utilization even looks reasonable, not pinned at
100%, yet throughput refuses to climb any further no matter how many
more threads join in. Profiling each thread individually shows nothing
alarming — no obvious hot function, no lock contention, no branch
mispredictions. The threads are, individually, doing exactly what
they're supposed to, as fast as any one of them ever could.

The ceiling wasn't in any thread's code at all. It was in how much data
the memory system as a whole — shared across every core on the chip —
could physically move per second. Every additional thread was correctly
computed, correctly scheduled, and starved of data at almost exactly the
same aggregate rate as the thread before it, because they were all
drawing from the same finite well.

## Predict Before Measuring

Before reading further: for a workload that reads a large buffer
sequentially and does very little computation per byte read, do you
expect throughput to keep scaling as thread count approaches (and
exceeds) the number of physical cores, or to flatten well before that
point? What would flattening early, rather than late, tell you about
what's actually constrained?

## Worked Example

Three shapes worth telling apart, all covered by earlier chapters from
different angles and unified here: **pointer chasing** (Chapters 16-17)
touches memory constantly but is *latency-bound* — each access must
wait for the previous one to complete, so total bytes moved per second
stays low no matter how "busy" the memory system looks, because there's
never more than one outstanding request at a time. **Streaming copy or
summation** (this chapter's `bandwidth` mode) is *bandwidth-bound* —
independent, prefetchable accesses let many requests be in flight at
once, and throughput is limited by how fast the memory channels can
physically deliver bytes, not by any single access's latency. A
**compute-heavy loop** doing significant arithmetic per byte read is
often neither — its ceiling is the CPU's execution throughput (Part II's
territory), with memory traffic low enough relative to computation that
it's not the constraint at all. Which of the three describes a given
workload determines which chapters' tools are even relevant to
improving it.

## Core Intuition

**Bytes transferred** is exactly what it says: the total data moved
between memory and the CPU over some measurement. **Sustained
bandwidth** is bytes transferred per second under real, continued load;
**peak bandwidth** is a hardware specification — the theoretical maximum
a memory system could ever deliver, essentially never achieved in
practice. The canonical picture is lanes carrying bytes per second:
more lanes (channels) or a faster flow rate raises how much can move
per second, but that says nothing about how long any *one* byte's trip
takes — bandwidth and latency (Chapter 16) are the lane-capacity and
trip-time of the same road, not the same measurement. **Arithmetic intensity** is the ratio of useful computation to
bytes transferred — operations per byte — and is what separates a
compute-bound workload (high intensity) from a bandwidth-bound one (low
intensity), for the same total bytes moved. **Memory-level parallelism**
is how many outstanding memory requests a workload can keep in flight at
once — pointer chasing has essentially none (each request depends on
the last); streaming access has a great deal (many independent
addresses can be requested before any of them return). **Bandwidth
saturation** is the point past which more concurrent demand cannot
increase delivered throughput, because the memory channels are already
moving data near their sustainable rate — this chapter's opening story,
directly. The **roofline intuition** ties these together informally: a
workload's achievable performance is bounded either by peak compute
throughput or by achievable bandwidth times arithmetic intensity,
whichever ceiling is lower for that specific workload — not a formal
model this book derives in full, but a mental picture worth carrying:
every workload has *some* ceiling, and knowing which one applies
determines what kind of change could possibly help.

## Technical Explanation

The reason memory-level parallelism matters so much: a single pointer
chase can never exceed one outstanding request's worth of progress per
round trip to memory, no matter how fast the memory channels
themselves are — the bottleneck is the *dependency*, not the hardware's
raw capacity. A streaming access pattern removes that dependency
entirely, letting the CPU and memory controller issue many requests
before any of them complete, which is precisely what lets sustained
bandwidth approach what the hardware can actually deliver. This is why
Chapter 16's pointer-chase tool and this chapter's streaming tool
measure genuinely different things even though both continuously access
memory: one is intentionally latency-bound to isolate latency; the
other is intentionally bandwidth-friendly to isolate bandwidth. Neither
tool is "better" — they answer different questions, the same lesson
Chapter 11 established for counting, sampling, and tracing, now applied
to memory access patterns.

## Tool View

- What is measured: aggregate sustained bandwidth (GB/s) across
  increasing thread counts, using `cyclelab bandwidth`, each thread
  streaming through its own large buffer.
- What is not measured: which specific resource saturates first (a
  memory controller, an interconnect link, a specific channel) — that
  needs vendor-specific uncore tooling (Chapter 20).
- Required permissions: none.
- Likely overhead: negligible for the measurement itself.
- Portability: works anywhere `cyclelab` runs; absolute GB/s numbers are
  entirely hardware-specific (memory generation, channel count, core
  count) — only the qualitative saturation shape transfers between
  machines.
- Common failure mode: seeing flat CPU utilization percentages during a
  bandwidth-saturated run and concluding nothing is wrong, because
  utilization measures whether a core is busy, not whether it's making
  forward progress waiting on data — a direct echo of Chapter 1's
  opening problem, now with memory bandwidth as the invisible
  constraint instead of a lock or a downstream call.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch19_bandwidth_scaling.sh
```

This sweeps `--threads` for `cyclelab bandwidth` at a fixed 64MB
per-thread working set, tabulating aggregate sustained bandwidth.

**Expected qualitative result:** bandwidth should rise close to linearly
at low thread counts, then flatten well before reaching the machine's
full logical core count. One example run on the reference machine for
this book (Apple M4, macOS, arm64, 10 logical CPUs) showed:

```text
threads  bandwidth_gb_s
1        14.84
2        26.93
4        51.31
6        59.14
8        75.80
10       74.11
```

Roughly linear scaling from 1 to 4 threads (14.84 to 51.31 GB/s, close
to 4x), then clear flattening from 6 threads onward — 8 and 10 threads
delivered essentially the same throughput.

**Interpretation:** compare the single-thread number (14.84 GB/s) against
`random-memory` mode's *implied* bandwidth at the same 64MB working set:
Chapter 16's data showed roughly 93.8ns per 64-byte access there, which
works out to about 0.68 GB/s — roughly 22x less than this chapter's
single-thread streaming number, despite both continuously accessing
memory the entire time. That gap *is* latency-bound versus
bandwidth-bound, made concrete: the pointer chase's lack of
memory-level parallelism, not any hardware limitation, is what caps it
so far below what the same hardware sustains for a friendlier access
pattern.

**Fallback path:** if `python3` isn't available, run the six
`cyclelab bandwidth --threads=...` commands directly and read
`results.bandwidth_gb_per_s` from each run's raw JSON.

**Cleanup:** none.

## Common Misconceptions

There is no blueprint-seeded misconception registry entry specific to
this chapter; a new one (M35, proposed) is worth naming directly, extending M02 (Chapter 1)
into memory-bandwidth territory: **"Flat CPU utilization during a
bandwidth-saturated workload means nothing is wrong."** This is wrong
because a core can be technically "busy" (not idle, not asleep) while
making very little forward progress, stalled waiting for data the
memory system can't deliver any faster — utilization measures busy time,
not useful throughput, exactly Chapter 1's original caution, now applied
to a specific, common real cause. The evidence that distinguishes the
two: this chapter's own lab — adding threads past the saturation point
does not increase completed work, even though every added thread is
technically running.

## Practical Implications

Before adding more parallelism to a workload that streams through large
amounts of memory with little computation per byte, check whether it's
already bandwidth-bound — this chapter's lab is the direct test.
Additional threads past that point add cost (scheduling, cache
pressure, coherence traffic from Chapter 18) without adding throughput,
the same shape as Chapter 1's low-CPU-utilization story but manifesting
as a saturated-but-not-obviously-so resource instead of an idle one.

## Key Takeaway

**A workload is bandwidth-bound when useful work cannot increase
because the memory system is already moving data near its sustainable
rate.**

## What to Remember

- Sustained bandwidth is what a workload actually achieves under load;
  peak bandwidth is a specification essentially never reached in
  practice.
- Arithmetic intensity (operations per byte) separates compute-bound
  workloads from bandwidth-bound ones at the same data volume.
- Memory-level parallelism — how many requests can be in flight at
  once — is what separates latency-bound access (pointer chasing) from
  bandwidth-bound access (streaming), even when both touch memory
  continuously.
- Bandwidth saturation shows up as throughput flattening despite more
  threads and non-idle CPU utilization — not as an obvious error.
- The roofline intuition: a workload's ceiling is either compute
  throughput or bandwidth times arithmetic intensity, whichever is
  lower for that specific workload.
- Absolute bandwidth numbers are entirely hardware-specific; only the
  qualitative rise-then-flatten shape transfers between machines.

## Further Reading

- Samuel Williams, Andrew Waterman, David Patterson, "Roofline: An
  Insightful Visual Performance Model for Multicore Architectures,"
  *Communications of the ACM*, 2009 — the formal roofline model this
  chapter's intuition is deliberately a simplified preview of.

## The Next Obvious Question

Which tools reveal cache and memory behavior?
