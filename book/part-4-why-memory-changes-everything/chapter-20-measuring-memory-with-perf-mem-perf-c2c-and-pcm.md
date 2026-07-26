# Measuring Memory with `perf mem`, `perf c2c`, and PCM

**Part:** Part IV — Why Memory Changes Everything
**Concept level:** 4
**Prerequisites:** shared cache line, coherence traffic (Chapter 18); bytes transferred, sustained bandwidth (Chapter 19)
**New concepts:** load/store sampling, data source, local/remote classification, cache-to-cache transfer, uncore, memory controller

## Opening Question

Which tools reveal cache and memory behavior?

## Incident or Real-World Story

A team suspects their service's slowdown is memory-related — plausible,
given everything Chapters 16 through 19 just established — and asks for
"the memory bottleneck report." No such single report exists. What
exists instead is a chain of increasingly specific, increasingly
narrow-scope tools: a controlled experiment can confirm the *shape* of
the problem (latency-bound? bandwidth-bound? false sharing?) without
identifying a specific line of code; a sampling profiler extended to
tag memory-related samples can point at specific instructions; a
cache-to-cache-transfer tool can confirm whether flagged sharing is
true or false; a vendor uncore tool can show whether specific memory
channels are saturated. No one tool in that chain says "here is the
memory bottleneck" on its own — each narrows the question the next
tool should even be asked.

The team that expects one authoritative report walks away frustrated.
The team that expects a ladder — each rung answering a more specific
question than the last, informed by what the previous rung showed —
gets somewhere.

## Predict Before Measuring

Before reading further: given everything Chapters 16 through 19 already
taught you to measure without any specialized memory-profiling tool
(latency by working-set size, latency by stride, throughput by padding,
bandwidth by thread count), what do you expect a dedicated hardware
tool like `perf mem` to add on top of that, that a controlled experiment
alone cannot show?

## Core Intuition

This chapter is organized as a **tool ladder**, each rung narrower and
more specific than the last:

1. **Controlled access-pattern experiments** — exactly Chapters 16-19's
   guided labs. No specialized tool, no permissions, works everywhere;
   answers *shape* questions (latency- or bandwidth-bound? does layout
   matter?) but never points at a specific line of code.
2. **`perf stat` with architecture-appropriate events** (Chapter 10) —
   cache-miss and memory-related hardware counters for a whole run,
   still no location information.
3. **`perf mem`** — samples individual load/store instructions and
   tags each with a **data source**: which cache level or memory
   satisfied it, including **local/remote classification** on NUMA
   systems (Part V's subject). This is **load/store sampling**: unlike
   `perf record`'s instruction-pointer sampling (Chapter 12), it samples
   *memory accesses* specifically, attributing cost to the exact
   instruction responsible.
4. **`perf c2c`** — built for exactly Chapter 18's problem: detecting
   **cache-to-cache transfers**, the HITM-style events that distinguish
   true sharing from false sharing, and pointing at the specific
   variables and threads involved.
5. **Uncore PMUs and vendor tools** (Intel PCM and equivalents) — counters
   outside the CPU cores themselves, in the **uncore** (shared cache,
   interconnect, and **memory controller** logic), for socket- or
   channel-level bandwidth and saturation detail beyond what
   Chapter 19's black-box throughput measurement can show.
6. **Application counters and source-level instrumentation** — the last
   resort when hardware visibility runs out: manual counters compiled
   into the code itself.

## Technical Explanation

Each rung trades portability and ease of use for specificity. Rung 1
(this book's actual, tested labs throughout this Part) works on any
machine with a C compiler and answers real questions, but never says
*which line*. Rungs 3 and 4 require Linux, hardware counter access
(Chapter 10's permissions model again), and CPU-specific event support
that varies by vendor and generation — real limitations, not
afterthoughts. Rung 5 often requires vendor-specific software and,
depending on platform, elevated privileges beyond even what `perf`
needs. The chain is only as useful as the hypothesis driving it:
without Chapters 16-19's shape-level understanding first, a `perf c2c`
report is a list of addresses with no context for which ones matter.

## Tool View

- What is measured: this chapter's portable lab is a deliberate
  synthesis exercise — rereading Chapters 16-19's own results as rung 1
  of this ladder, then stating precisely what each higher rung would
  add.
- What is not measured: rungs 2-6 are not executed on this book's
  reference machine (macOS, no `perf`, no Intel PCM support on Apple
  Silicon) — every command below is documented against each tool's
  stable, published interface, not tested against real captured output.
- Required permissions: rung 1, none. Rungs 2-4, Linux with hardware
  counter access (Chapter 10). Rung 5, often root or a vendor driver,
  varying by tool and platform.
- Likely overhead: load/store sampling (`perf mem`) and `perf c2c`
  typically carry more overhead than plain instruction-pointer sampling,
  since every sampled event carries a memory address and classification
  to record — budget for it the same way Chapter 11 taught for tracing.
- Portability: rungs 2 onward are Linux-only and CPU-vendor-specific in
  their exact event names (Appendix C); rung 1 is fully portable.

  ```bash
  # Rung 3: perf mem, sequential vs. random access (Chapter 16-17's own comparison)
  perf mem record -- ./labs/cyclelab/bin/cyclelab random-memory --working-set-size=64M --duration=2
  perf mem report

  # Rung 4: perf c2c, packed vs. padded counters (Chapter 18's own comparison)
  perf c2c record -- ./labs/cyclelab/bin/cyclelab false-sharing --padding=packed --threads=8 --duration=2
  perf c2c report
  ```

  **Documented, not tested** on this book's macOS reference machine.
- Common failure mode: reaching for `perf c2c` (or any rung-4+ tool)
  before establishing, via rung 1 or 2, that there's a sharing-shaped
  problem to look for in the first place — an expensive, narrow tool
  used as a first move instead of a confirming one.

## Guided Lab

**Portability:** portable (rung 1, this chapter's actual lab); rungs
3-6 are **hardware-dependent** / **privileged** / Linux-only, documented
above but not exercised.

**Setup:** none beyond having already run Chapters 16-19's labs.

**Exercise:** for each of the four results you already collected in this
Part, write down which rung of this chapter's ladder produced it, and
what the *next* rung up would add:

1. Chapter 16's working-set-size latency staircase — rung 1. Rung 3
   (`perf mem`) would add: which specific instruction in a real,
   larger program is generating the samples landing in the slow tier,
   not just that a slow tier exists.
2. Chapter 17's stride sweep — rung 1. Rung 3 would add: the actual data
   source classification (L2 hit vs. LLC hit vs. DRAM) per sample,
   instead of inferring tier from aggregate latency alone.
3. Chapter 18's packed-vs-padded comparison — rung 1. Rung 4
   (`perf c2c`) would add: direct confirmation that the packed case's
   slowdown is specifically cache-to-cache transfer traffic (true HITM
   events), not some other effect that happened to correlate with
   layout.
4. Chapter 19's thread-count bandwidth sweep — rung 1. Rung 5 (uncore
   tools) would add: which specific memory channel or interconnect link
   is the saturating resource, rather than only observing that
   *aggregate* throughput stopped scaling.

**Expected qualitative result:** every rung-1 result from this Part
already answered a real, defensible question; every higher rung adds
*location* or *confirmation* detail without changing the underlying
conclusion rung 1 already supported.

**Interpretation:** this is the chapter's key takeaway made concrete —
none of Chapters 16-19's conclusions were wrong or incomplete for
lacking `perf mem`/`perf c2c`/PCM data; they were appropriately scoped
to what a portable, hypothesis-driven measurement could support, with
the higher rungs available for exactly the cases (unfamiliar codebase,
need to point at a specific line, need to confirm true vs. false
sharing definitively) where more specificity is actually needed.

**Fallback path:** this entire chapter's lab is already the fallback
path — no rung-3-or-higher tool is required to complete it.

**Cleanup:** none.

## Common Misconceptions

There is no blueprint-seeded misconception registry entry specific to
this chapter; a new one (M36, proposed) is worth naming directly, closing out this Part:
**"A single memory-related counter or tool can report 'the memory
bottleneck.'"** This is wrong because memory behavior spans latency,
bandwidth, coherence, and topology (Part V's subject next), each
requiring a different measurement, and no single number aggregates all
of them meaningfully — this chapter's whole ladder structure exists
because the real answer is always a chain of partial, scoped
observations, not one report. The evidence that distinguishes the two:
this chapter's own exercise — four genuinely different Part IV findings
(latency, stride, coherence, bandwidth), none reducible to a shared
single metric.

## Practical Implications

Before reaching for a specialized memory tool, check whether a
controlled, portable experiment in the shape of Chapters 16-19's labs
already answers the question — and when a specialized tool genuinely is
needed, let the portable experiment's result determine *which* rung to
reach for next, rather than defaulting to the most powerful tool
available.

## Key Takeaway

**Memory analysis is a chain of partial observations; no single counter
reports "the memory bottleneck" for every machine.**

## What to Remember

- The tool ladder runs from portable controlled experiments through
  `perf stat`, `perf mem`, `perf c2c`, uncore/vendor tools, to
  application-level instrumentation — each rung narrower and more
  specific than the last.
- `perf mem` samples individual memory accesses and classifies their
  data source (which cache level, or local/remote memory, satisfied
  each one) — location information rung 1 experiments cannot provide.
- `perf c2c` is built specifically to distinguish true from false
  sharing by detecting cache-to-cache transfer (HITM) events directly.
- Uncore PMUs and vendor tools (Intel PCM and equivalents) expose
  socket- and channel-level detail beyond what application-level
  throughput measurement can show.
- Higher rungs cost more in permissions, portability, and overhead —
  reach for them once a lower rung has established there's a specific
  question worth that cost.
- No single tool or counter reports "the" memory bottleneck; real
  memory analysis is always a chain of scoped, partial observations.

## Further Reading

- Linux perf manual pages (`perf mem`, `perf c2c`):
  <https://man7.org/linux/man-pages/man1/perf.1.html>
- Intel Performance Counter Monitor: <https://github.com/intel/pcm>

## The Next Obvious Question

How does Linux decide where runnable work executes?
