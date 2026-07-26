# Chapter 17 — Working Sets, Cache Misses, and Prefetching

**Part:** Part IV — Why Memory Changes Everything
**Concept level:** 4
**Prerequisites:** cache line, L1/L2/LLC, latency, locality (Chapter 16)
**New concepts:** working set, cache hit, compulsory miss, capacity miss intuition, stride, reuse, prefetcher, TLB as a forward pointer

## Opening Question

Why do access order and working-set size change performance?

## Incident or Real-World Story

A team ports a numeric routine from one language to another and finds
the new version, despite using an algorithmically identical nested loop
over a large 2D array, runs several times slower. Nobody changed the
amount of arithmetic. What changed, buried in the translation, is which
index varies fastest in the inner loop — the original walked the array
in the order it's actually laid out in memory (row by row); the port
walks it column by column, the same total set of elements, visited in a
different order. Every element is still touched exactly once. The
*order* alone is enough to turn a cache-friendly loop into one that
defeats the hardware's ability to help at all.

This is Chapter 16's locality principle made concrete and, importantly,
made controllable: the data didn't change, the working set's *size*
didn't change, only the *order* of access — and order alone was enough
to dominate the result.

## Predict Before Measuring

Before reading further: for the same total working-set size, held
constant, do you expect access *order* alone to change per-access
latency by a small amount (tens of percent) or a large amount
(multiples)? Chapter 16 varied working-set size at a fixed (random)
order; this chapter's lab varies order at a fixed (large) size — hold a
prediction for which matters more.

## Worked Example

Row-major versus column-major traversal of a 2D array is this chapter's
canonical example, and it maps directly onto a simpler primitive: a
fixed **stride** through a flat buffer. Row-major traversal, walking
each row before moving to the next, is a stride-1 walk through memory
as it's actually laid out — maximal spatial locality, ready-made for a
hardware **prefetcher** (circuitry that speculatively loads data it
predicts will be needed soon, based on recently observed access
patterns) to recognize and act on. Column-major traversal of the same
array, jumping by the row length on every step, is a large-stride walk —
each access lands in a different cache line, mostly defeating both
locality and prefetching. Same data, same total accesses, dramatically
different cost, purely from stride.

## Core Intuition

A **working set** is the specific data a program actually touches
repeatedly during some phase of execution — not all the memory it
could touch, but what it's actively using right now. A **cache hit** is
an access satisfied by a cache level instead of going further out; a
**compulsory miss** is the unavoidable first-ever access to an address
that could not have been cached yet no matter how good the access
pattern is; **capacity miss intuition** is the informal sense that
misses beyond the compulsory minimum happen because the working set
exceeds what a cache level can hold, forcing useful data out to make
room. **Reuse** is accessing the same data again while it's still cached
— the thing temporal locality rewards and poor access order defeats. A
**TLB** (translation lookaside buffer) is worth introducing here as **a
forward pointer**: a small cache of virtual-to-physical address
translations, itself subject to the same hit/miss dynamics as a data
cache, and itself capacity-limited in a way that large-stride access
can exhaust independently of the data cache.

## Technical Explanation

Stride determines how much of a prefetcher's help a workload can
actually collect, but the relationship is not simply "smaller stride is
always better, larger is always worse." Real prefetchers implement
specific pattern-detection logic — commonly tuned around small,
constant strides seen recently — and different stride values can
interact with that detection logic in ways that are not monotonic:
some medium strides may confuse a stream detector more than a
somewhat larger, more obviously non-unit stride does. This chapter's
lab shows this directly rather than asserting a clean theoretical
curve, because real hardware doesn't always provide one, and the
honest lesson is more useful than an oversimplified one: contiguous
(stride-1) access is dramatically better than *any* strided access on
real hardware, but ranking non-unit strides against each other requires
measuring the specific hardware, not applying a general rule.

## Tool View

- What is measured: latency per access at a fixed, larger-than-cache
  working set, swept across increasing stride values, using
  `cyclelab sequential-memory --stride=N`.
- What is not measured: which specific miss type (compulsory vs.
  capacity) or which cache level accounts for the cost at any given
  stride — that requires hardware counters (`perf mem`, Chapter 20) this
  chapter's portable lab doesn't require.
- Required permissions: none.
- Likely overhead: negligible; the measurement is the point.
- Portability: works anywhere `cyclelab` runs; exact prefetcher behavior
  is architecture- and even model-specific — treat any particular
  stride's relative cost as a fact about this run's hardware, not a
  general rule.
- Common failure mode: assuming a monotonic "stride vs. cost" curve and
  distrusting a real, correctly-measured non-monotonic result — this
  chapter's own lab data is exactly such a case.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch17_stride_sweep.sh
```

This sweeps `--stride` from 1 to 1024 (in 64-byte slots) at a fixed
64MB working set — well beyond this book's reference machine's
last-level cache, so every access is a genuine memory-hierarchy
traversal, isolating stride as the only variable.

**Expected qualitative result:** stride=1 should be dramatically faster
than every other stride tested; strides beyond 1 should all cost
several times more than stride=1, but should *not* necessarily form a
smooth, increasing curve as stride grows. One example run on the
reference machine for this book (Apple M4, macOS, arm64) showed:

```text
stride   ns_per_access
1        2.199
2        10.569
4        18.440
8        34.156
16       12.268
32       26.040
64       46.319
128      51.189
256      18.321
```

Stride=1 at 2.2ns versus every other stride at 10-51ns — roughly 5x to
23x worse from access order alone, at *identical* working-set size. Note
the non-monotonic middle: stride=16 (12.3ns) is faster than stride=8
(34.2ns), and stride=256 (18.3ns) is faster than stride=128 (51.2ns).

**Interpretation:** the gap between stride=1 and everything else is the
headline result and should replicate on most hardware with a stream
prefetcher. The specific non-monotonic shape between stride=2 and
stride=1024 is this machine's prefetcher's specific behavior — expect a
different, but likely still non-monotonic, shape on a different CPU,
and do not treat any single non-unit stride's relative ranking as a
portable fact.

**Fallback path:** if `python3` isn't available, run the eleven
`cyclelab sequential-memory --stride=...` commands directly and read
`results.ns_per_access` from each run's raw JSON.

**Cleanup:** none.

## Common Misconceptions

**M05 — "A high cache-miss percentage proves a cache bottleneck."** This
is wrong because a miss rate needs access volume, miss cost, and
overlap with other work to mean anything on its own — a workload with a
high miss rate but few total accesses can matter less than one with a
lower miss rate but far more accesses, and misses that overlap with
useful computation elsewhere cost less than misses that block progress
entirely. The evidence that distinguishes the two: connect a miss
count (or, as in this chapter's portable lab, an elapsed-time proxy for
it) to total access volume and completed work before concluding
anything is a "bottleneck" — a phrase this chapter's own numbers earn
only when paired with how much work those accesses were part of.

## Practical Implications

Before restructuring a loop for "better cache behavior," check whether
its access order actually has the locality its author assumes — as in
this chapter's incident, a seemingly neutral change (like a language
port that silently swaps loop nesting) can turn a contiguous access
pattern into a strided one without a single line of arithmetic
changing. When reasoning about a real miss-rate number from a profiler,
always pair it with access volume and completed work, not the
percentage alone.

## Key Takeaway

**Caches reward reuse and predictable access; miss counts become
meaningful only when connected to access volume, latency, and completed
work.**

## What to Remember

- A working set is the data actually being reused right now, not
  everything a program could touch.
- Compulsory misses are unavoidable; misses beyond that reflect a
  working set exceeding a cache level's capacity, an intuition worth
  having even without literally classifying each miss.
- The TLB is a forward pointer for address translation, itself
  cache-like and itself capacity-limited independently of the data
  cache.
- Access order (stride) can dominate performance at a fixed working-set
  size, exactly as size dominated at a fixed (random) order in Chapter
  16.
- Real prefetcher behavior across different strides is not necessarily
  monotonic — contiguous access is reliably best, but ranking non-unit
  strides requires measuring the actual hardware.
- A miss-rate percentage alone is not a verdict; it needs access volume
  and completed work to mean anything (M05).

## Further Reading

- Ulrich Drepper, "What Every Programmer Should Know About Memory,"
  2007 — prefetching and TLB sections directly relevant here.

## The Next Obvious Question

How can independent threads slow each other through cache coherence?
