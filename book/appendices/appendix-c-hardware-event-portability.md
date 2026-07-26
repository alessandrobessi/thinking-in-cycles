# Appendix C — Hardware Event Portability

**Status:** reference material, not a chapter. Expands on Chapter 10's
introduction of the PMU and hardware performance events with the
portability detail that chapter deliberately left out to keep its own
narrative focused.

## Generic events

Most profiling tools (`perf` included) expose a small set of **generic
events** — `cycles`, `instructions`, `cache-references`,
`cache-misses`, `branch-misses`, and a handful of others — that the
tool itself maps to whatever the underlying hardware's real,
vendor-specific counter is. This is precisely why `cyclelab compute`'s
IPC intuition (Chapter 7) and `perf stat`'s default event set (Chapter
10) work the same way conceptually across an Intel, AMD, or Arm
machine, even though the actual silicon underneath each generic name is
different hardware entirely. Generic events are the right default for
any cross-architecture comparison or any teaching example — exactly why
this book's own narrative (Chapters 6-10) stays in generic-event terms
throughout — but they necessarily can't expose anything a generic name
doesn't cover.

## Raw events

A **raw event** bypasses the generic mapping and names a specific
hardware counter directly, by its vendor-documented event code (on
Linux `perf`, something like `perf stat -e r04a3`, an
architecture-specific hex code with no portable meaning at all).
Raw events are necessary once a question needs something no generic
alias covers — a specific cache level's fill-from-DRAM count, a
specific stall reason, a specific uncore counter (Chapter 20) — but the
event code itself is meaningless without checking the exact
microarchitecture's own event list first (Appendix A: "every
hardware-specific event description must be checked against the
relevant architecture documentation," this project's own source-policy
rule). A raw event that means one thing on one CPU
generation can mean something entirely different, or nothing at all, on
the next.

## Event aliases

Between fully generic and fully raw sits the **event alias**: a
tool-maintained, human-readable name for a specific, non-generic
hardware event (Linux `perf list` shows these alongside the generic
set, distinguishable by more specific names like
`branch-instructions.speculative`). Aliases are worth preferring over
raw hex codes whenever one exists for the question at hand — same
specificity, without needing to hand-decode a vendor manual — but the
alias itself is still tied to the tool's own knowledge of that specific
PMU model, so it shares raw events' portability limits, just with a
friendlier name.

## PMU models

The **PMU** (performance monitoring unit) itself differs meaningfully
by microarchitecture generation, not just by vendor — two CPUs from the
same vendor two generations apart can have different counter widths,
different available raw events, and different constraints on which
events can be counted simultaneously. This is the practical reason
Chapter 10's own guided lab treats every raw or aliased event's meaning
as something to verify against the *specific* running CPU
(`cat /proc/cpuinfo` for the model, then that model's own optimization
manual — Appendix A's "matching tools to hardware" principle applied to
events specifically) rather than assumed from a similar-sounding CPU's
documentation.

## Multiplexing

A CPU has a fixed, small number of physical counter registers — often
around four general-purpose counters per logical CPU, though this
varies by microarchitecture — and requesting more simultaneous events
than that forces the tool to **multiplex**: rotating which events are
actually counted moment to moment and then scaling the results back up
to estimate what a full-time count would have shown. Chapter 10's own
guided lab demonstrates this directly: an event group small enough to
fit in available counters reports exactly; a larger one reports a
`scaled` or `enabled/running` ratio below 100%, meaning the number
shown is an estimate, not a direct count. This is why Chapter 10 treats
"check for multiplexing" as a Definition-of-Done-level requirement
before trusting a `perf stat` number, not an edge case.

## Hybrid CPUs

This book's own reference machine is a genuinely hybrid CPU — Apple
Silicon, with 4 performance cores and 6 efficiency cores on the machine
used throughout this book (confirmed directly: `sysctl -n
hw.perflevel0.physicalcpu` reports 4, `hw.perflevel1.physicalcpu`
reports 6). Intel's P-core/E-core hybrid designs raise exactly the same
issue on Linux: the two core types frequently have *different* PMUs
with different available events and sometimes different counter
widths, and a workload's threads migrating between core types mid-run
(Chapter 23's own affinity material) can make a single aggregate event
count represent a blend of two different hardware realities rather than
one consistent measurement. Pinning to a single core type (where hard
affinity is available — Chapter 23 documents this book's own reference
machine as lacking that unprivileged control on macOS) is the direct
mitigation whenever a hybrid CPU's per-core-type PMU differences would
otherwise confound a comparison.

## Precise event-based sampling: Intel PEBS, AMD IBS, Arm SPE

Ordinary hardware-event sampling (Chapter 11) has a real, structural
skid: the event that triggers a sample and the instruction pointer
actually recorded can be several instructions apart, because of
out-of-order execution and pipeline depth (Chapter 8) — the CPU doesn't
stop and record the exact retiring instruction the moment a counter
overflows. Each major architecture has its own answer to this:
**Intel PEBS** (Precise Event-Based Sampling) and **AMD IBS**
(Instruction-Based Sampling) both use dedicated hardware support to
capture a much more precisely attributed sample, including extra
context like data addresses for memory-related events; **Arm SPE**
(Statistical Profiling Extension) is Arm's equivalent, available on
some — not all — Arm server and mobile cores. None of the three is
universally available (it depends on the specific CPU model, and on
this book's own reference machine — Apple Silicon — Arm SPE support is
not exposed to user-space profiling tools at all), so a profiler's own
documentation needs checking for whether it uses precise sampling by
default, and whether the running hardware actually supports it, before
assuming sample attribution is more precise than Chapter 11's ordinary
statistical-sampling caveats already describe.

## Related

- Chapter 7 (cycles, instructions, IPC as generic-event territory),
  Chapter 10 (`perf stat`, event groups, multiplexing), Chapter 11
  (sampling's attribution skid, motivating PEBS/IBS/SPE), Chapter 20
  (uncore/memory-controller events, usually raw), Chapter 23 (CPU
  affinity, relevant to hybrid-CPU PMU differences).
- Appendix A ("matching tools to hardware" generally); Appendix B (the
  `perf list` command that surfaces generic events and aliases
  together).
