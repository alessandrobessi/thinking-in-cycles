# The Memory Hierarchy and Locality

**Part:** Part IV — Why Memory Changes Everything
**Concept level:** 4 (Level 0-3 concepts from Parts I-III assumed)
**Prerequisites:** cycle, instruction, stall, memory wait (Part II); sampling, call graph (Part III)
**New concepts:** register, cache line, L1, L2, last-level cache, DRAM, latency, locality, spatial locality, temporal locality

## Opening Question

Why can memory access dominate code that performs little computation?

## Incident or Real-World Story

An engineer rewrites a hot loop to use noticeably fewer arithmetic
operations — by the count of instructions alone, a clear improvement —
and is surprised to find the new version runs no faster, and on some
inputs, measurably slower. The instruction count went down exactly as
predicted. What the instruction count doesn't show is where each of
those instructions was spending its time waiting: the original loop
walked through a large array in a pattern that happened to reuse recently
touched data; the "improved" version, despite doing less arithmetic per
element, touched memory in a wider, less predictable pattern. Almost
none of the loop's time was ever going to arithmetic in either version —
it was going to waiting for data to arrive from memory, and the rewrite
made that wait longer, not shorter.

This is the pattern Chapter 6 already named in the abstract — the CPU
executes the compiler's instruction stream, not the programmer's mental
model of the source — extended here to a specific, common way that
mental model goes wrong: assuming instructions are the expensive part of
a program, when for a large share of real workloads, memory access is.

## Predict Before Measuring

Before reading further: if you access the same total number of bytes
from memory, but do it in a pattern that reuses recently touched data
heavily versus a pattern that scatters accesses across a huge range with
no reuse, do you expect the *per-access* cost to be the same, moderately
different, or dramatically different? Hold your answer for this
chapter's Guided Lab, which measures exactly this.

## Worked Example

A useful, if imperfect, picture: a desk holds what you're working with
right now; a nearby drawer holds things you'll likely need again soon;
a room-sized archive down the hall holds everything from this project;
a remote warehouse holds everything else the organization owns. Each
step outward holds more but costs more time to reach. The analogy breaks
in an important way worth stating immediately: unlike a desk and a
drawer, a CPU's cache levels are not manually organized by the
programmer — what ends up in each level is a consequence of *access
pattern*, decided automatically by hardware based on what was recently
or predictably used, not by deliberate placement.

## Core Intuition

A **register** is the fastest storage a CPU has, directly wired into its
execution units — vanishingly small in capacity, effectively free to
access. A **cache line** is the unit caches actually move data in
(commonly 64 bytes) — touching even one byte pulls in the whole line.
**L1**, **L2**, and **last-level cache** (LLC) are progressively larger,
progressively slower layers of on-chip memory sitting between registers
and main memory; **DRAM** is main memory itself — much larger than any
cache level, and much slower to reach. **Latency**, in this context, is
the time a single memory access takes to complete, which grows sharply
at each layer further from the CPU. **Locality** is the general
principle that makes caching effective at all: **spatial locality** is
the tendency for nearby addresses to be accessed close together in time
(so pulling in a whole cache line pays off); **temporal locality** is
the tendency for the same address to be accessed again soon (so keeping
recently used data around pays off). Both are properties of *how a
program accesses memory*, not properties of the hardware — which is
exactly why the same hardware can be fast for one access pattern and
slow for another.

## Technical Explanation

The mandatory distinction this chapter insists on: cache **capacity**
(how much a level holds), cache **latency** (how long one access to that
level takes), and memory **bandwidth** (how many bytes per second can
move once a stream of accesses gets going) are related but genuinely
different properties, easy to conflate. A workload can fit comfortably
within a cache level's capacity and still suffer if its *access pattern*
defeats locality (Chapter 17's subject in full). A workload can have
excellent per-access latency and still be bandwidth-limited once enough
concurrent accesses are in flight (Chapter 19's subject). This chapter
isolates latency specifically: the Guided Lab measures nothing but *time
per access*, deliberately structured (via a dependent pointer chase,
explained in Tool View) so bandwidth and prefetching effects can't hide
the underlying cost.

## Tool View

- What is measured: average latency per memory access, swept across
  increasing working-set sizes, using `cyclelab random-memory`.
- What is not measured: bandwidth (Chapter 19) or the effect of access
  *order* specifically, as opposed to working-set size (Chapter 17).
- Required permissions: none.
- Likely overhead: negligible; this is the measurement itself, not an
  external observation of something else.
- Portability: works anywhere `cyclelab` runs; exact cache sizes and
  latencies are architecture- and even model-specific, so absolute
  numbers won't transfer between machines, only the qualitative
  staircase shape.
- Mechanism: `random-memory` mode builds a buffer of cache-line-sized
  slots connected by a single-cycle random permutation (built with
  Sattolo's algorithm, guaranteeing every slot is genuinely visited, not
  just a lucky few), then chases the pointer chain: each step's address
  depends on the *previous* step's result, so the CPU cannot prefetch
  ahead or reorder around the wait — a deliberate dependent-load chain,
  the same principle Chapter 8 used for arithmetic, now applied to
  memory specifically. This isolates latency from bandwidth and
  prefetching effects, which is exactly why it's the right tool for
  *this* chapter and the wrong one for Chapter 19's.
- Common failure mode: sweeping working-set size with a *sequential*
  access pattern instead and concluding cache sizes are far larger than
  they are — sequential access lets the prefetcher hide most of the
  latency this chapter is trying to isolate, which is itself Chapter
  17's subject, not a mistake to make here by accident.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch16_memory_hierarchy.sh
```

This sweeps `--working-set-size` from 16K to 128M using
`cyclelab random-memory`, tabulating nanoseconds per access.

**Expected qualitative result:** a staircase, not a smooth curve — flat
regions punctuated by sharp jumps at a small number of specific sizes.
One example run on the reference machine for this book (Apple M4,
macOS, arm64) showed:

```text
size    ns_per_access
16K     1.802
128K    1.602
256K    5.891
8M      8.202
16M     28.965
128M    102.724
```

Three distinct plateaus: roughly 1.6-1.8ns up to 128K, roughly 5-8ns
from 256K to 8M, and roughly 30-100ns from 16M upward — a visible L1/L2
boundary, a last-level-cache region, and a DRAM region.

**Interpretation:** do not expect these exact sizes or latencies on a
different CPU — cache sizes vary by vendor, generation, and product
tier, and this book deliberately keeps that detail architecture-specific
rather than universal (style guide: never claim a universal threshold).
The qualitative result — a small number of flat plateaus separated by
sharp jumps, not a smooth gradient — is what to look for. If your run
shows more than three or four distinct steps, that's plausible too
(some machines expose L1/L2/L3 as three separately visible plateaus);
fewer or more steps than this chapter's example is not itself a sign of
a bad measurement.

**Fallback path:** if `python3` isn't available, run the fourteen
`cyclelab random-memory --working-set-size=...` commands directly and
read `results.ns_per_access` from each run's raw JSON by eye.

**Cleanup:** none.

## Common Misconceptions

### *"A working set that fits in cache is automatically fast." (M33)*

**Why it's wrong:** Fitting in cache is necessary but not sufficient —
an access pattern that defeats locality (this chapter's opening story)
can still perform poorly even within a cache level's capacity, and
Chapter 17 shows this directly.

**Correct intuition:** This chapter's own lab already hints at it — the
jump from 128K to 256K happens at a specific *capacity* boundary, but
nothing in this chapter's measurement yet separates "does it fit" from
"is it accessed well," a distinction Chapter 17 makes explicit.

**Analogy:** A messy desk and a tidy desk can hold exactly the same
number of papers, but finding one specific document takes very
different amounts of time on each — fitting on the desk and being easy
to find are two different properties.

## Practical Implications

Before assuming a slow loop's problem is arithmetic, check whether its
*data* fits the working set your measurements suggest is fast on the
target machine, and whether its access pattern has spatial or temporal
locality to exploit. Reducing instruction count without considering
either can leave performance unchanged or make it worse, exactly this
chapter's opening story.

## Key Takeaway

**Memory performance depends not only on how much data is used, but on
when, where, and in what order it is accessed.**

## What to Remember

- Registers, L1, L2, last-level cache, and DRAM form a hierarchy of
  increasing capacity and increasing latency, in that order.
- A cache line, not a single byte, is the unit caches actually move —
  touching one byte pulls in its whole line.
- Spatial locality rewards accessing nearby addresses close together in
  time; temporal locality rewards reusing the same address soon.
- Cache capacity, cache latency, and memory bandwidth are related but
  different properties — fitting in a cache level doesn't guarantee good
  latency, and good latency doesn't guarantee good bandwidth.
- A dependent pointer chase (each step's address depends on the last)
  measures true latency by defeating prefetching and reordering — the
  right tool specifically because it removes the hardware's ability to
  hide the cost being measured.
- Reducing instruction count without considering data access pattern can
  leave a memory-bound workload's performance unchanged or worse.

## Further Reading

- Ulrich Drepper, "What Every Programmer Should Know About Memory,"
  2007 — the canonical deep treatment of cache hierarchy behavior,
  still broadly accurate in mechanism even as specific numbers have
  aged.

## The Next Obvious Question

Why do access order and working-set size change performance?
