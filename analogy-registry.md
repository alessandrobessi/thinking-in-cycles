# Analogy Registry

Seed table from this project's founding design notes, plus tracking
columns for actual use. Analogies should be reused consistently and
retired when they stop helping. Status as of this revision: Chapters
1–30 drafted — the full book is complete.

| Concept | Canonical analogy | Constraint | First used in | Status |
|---|---|---|---|---|
| Latency vs throughput | Travel time for one vehicle vs vehicles per hour | Do not imply roads perfectly model queues. | Chapter 3 | substituted |
| Saturation | Checkout line whose queue grows after cashiers reach capacity | Pair with measured latency curves. | Chapter 3 | used |
| CPU pipeline | Multi-stage assembly line with out-of-order readiness | Clarify that CPU execution is more dynamic than a literal conveyor belt. | Chapter 8 | used |
| Sampling | Periodically photographing a factory floor | Emphasize statistical representation, not complete history. | Chapter 11 | used |
| Flame graph | A population map of observed call stacks | Never call it a timeline. | Chapter 14 | used |
| Cache hierarchy | Desk, drawer, archive, warehouse | Use only to establish distance and capacity. | Chapter 16 | used |
| Branch prediction | Choosing a route before reaching a fork | Explain recovery cost after the analogy. | Chapter 9 | used |
| False sharing | Two people repeatedly erasing separate fields on one shared whiteboard | Emphasize cache-line granularity. | Chapter 18 | used |
| Memory bandwidth | Lanes carrying bytes per second | Distinguish lane capacity from trip latency. | Chapter 19 | used |
| CPU affinity | Assigning a worker to a workstation | Explain lost flexibility. | Chapter 23 | used |
| NUMA | Multiple workshops with local storerooms connected by a corridor | Keep CPU placement and memory placement distinct. | Chapter 24 | used |
| eBPF | Temporary sensors attached to defined points in a running machine | Sensors are constrained programs, not passive magic. | Chapter 27 | used |
| Benchmark | Controlled scientific experiment | Use literally: hypothesis, controls, repetitions, uncertainty. | Chapters 2, 4, 5 | used |

## Notes on current use

- **Saturation** (checkout line whose queue grows after cashiers reach
  capacity) is used in Chapter 3's Worked Example essentially verbatim
  against the seed analogy, paired directly with the chapter's own
  latency-curve discussion (its required constraint).
- **Latency vs throughput**'s seed analogy (vehicles/roads) was **not**
  used — Chapter 3's Worked Example teaches the same latency/throughput
  distinction entirely through the checkout-line analogy instead (one
  customer's wait = latency; cashiers' combined rate = throughput),
  which already does this work cleanly without needing a second,
  redundant analogy alongside it. Per Section 17's own "retired when
  they stop helping" allowance, this is recorded honestly as
  **substituted**, not silently marked "used" for an analogy whose
  specific imagery (vehicles, roads) never actually appears in the
  chapter. Caught during the Phase 7 analogy-consistency pass, which
  found this row's "used" status had been asserted without checking
  the chapter text against the seed analogy's own specific imagery.
- **Benchmark as controlled scientific experiment** is used literally
  starting in Chapter 2 — added during the Phase 7 analogy-consistency
  pass, which found the chapter originally never used the words
  "experiment"/"hypothesis" despite the registry claiming it did;
  Chapter 2's Practical Implications now states directly that defining
  the operation and metric before measuring is "that experiment's
  first, non-negotiable requirement," with Chapter 4 (repetition,
  control, uncertainty) and Chapter 5 (hypothesis/falsification)
  building out the rest of the same standing analogy.
- No new analogies were introduced in Chapters 1–30 beyond the seed list;
  Chapter 1's incident narrative is told directly, without a standing
  analogy, since the blueprint's Section 17 list has no entry for
  "on-CPU/off-CPU time accounting."
- **CPU pipeline** (assembly line) is used in Chapter 8's Worked Example,
  with the required constraint honored directly in the same paragraph
  ("Unlike a literal assembly line, ... out-of-order execution lets the
  CPU look ahead...").
- **Branch prediction** (choosing a route before a fork) is used in
  Chapter 9's Core Intuition, with the recovery-cost constraint honored
  via the "prepared to double back if this particular choice turns out
  wrong" clause in the same sentence.
- **Sampling** (photographing a factory floor) is used in Chapter 11's
  Core Intuition, with the constraint honored by pairing it directly
  with the "a photo... says almost nothing about a single rare, brief
  event between photos" caveat.
- **Flame graph** (population map of call stacks) is used in Chapter
  14's Core Intuition, with the "never call it a timeline" constraint
  honored in the same sentence and reinforced by that chapter's M06
  misconception entry.
- **Cache hierarchy** (desk, drawer, archive, warehouse) is used in
  Chapter 16's Worked Example, with the distance/capacity-only
  constraint honored by immediately stating where the analogy breaks
  ("unlike a desk and a drawer, a CPU's cache levels are not manually
  organized by the programmer").
- **False sharing** (two people erasing separate fields on one shared
  whiteboard) is used in Chapter 18's Core Intuition, with the
  cache-line-granularity constraint honored by the "the granularity
  that matters is the whiteboard... not the field" clause in the same
  sentence.
- **Memory bandwidth** (lanes carrying bytes per second) is used in
  Chapter 19's Core Intuition, with the lane-capacity-vs-trip-latency
  constraint honored directly by contrasting it with Chapter 16's
  latency measurement in the same sentence.
- **CPU affinity** (assigning a worker to a workstation) is used in
  Chapter 23's Core Intuition, with the lost-flexibility constraint
  honored by the "at the cost of the flexibility to shift them
  elsewhere" clause in the same sentence.
- **NUMA** (workshops with local storerooms connected by a corridor) is
  used in Chapter 24's Core Intuition, with the
  CPU-placement-vs-memory-placement constraint honored by an explicit
  cross-reference to Chapter 23's M10 in the same breath.

- **eBPF** (temporary sensors attached to defined points in a running
  machine) is used in Chapter 27's Core Intuition, with the
  constrained-programs-not-passive-magic requirement honored directly:
  "everything it can report was explicitly programmed in, checked for
  safety before installation (the verifier), and limited to the
  specific attachment point (the hook) it was built for."

Twelve of this project's thirteen seed analogies are in
active use as of Chapters 1–30, each verified directly against its
citing chapter's actual text during an analogy-consistency
pass (not just checked against this registry's own prior claims). The
"Latency vs throughput" analogy is the one exception: substituted by
Chapter 3's checkout-line analogy rather than used as specified. This
closes out the seed analogy list for the entire book.
