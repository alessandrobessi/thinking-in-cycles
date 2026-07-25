# Analogy Registry

Seed table reproduced verbatim from BLUEPRINT.md Section 17, plus tracking
columns for actual use. Per Section 17: analogies should be reused
consistently and retired when they stop helping. Status as of this
revision: Chapters 1–5 drafted.

| Concept | Canonical analogy | Constraint | First used in | Status |
|---|---|---|---|---|
| Latency vs throughput | Travel time for one vehicle vs vehicles per hour | Do not imply roads perfectly model queues. | Chapter 3 | used |
| Saturation | Checkout line whose queue grows after cashiers reach capacity | Pair with measured latency curves. | Chapter 3 | used |
| CPU pipeline | Multi-stage assembly line with out-of-order readiness | Clarify that CPU execution is more dynamic than a literal conveyor belt. | — | not yet used |
| Sampling | Periodically photographing a factory floor | Emphasize statistical representation, not complete history. | — | not yet used |
| Flame graph | A population map of observed call stacks | Never call it a timeline. | — | not yet used |
| Cache hierarchy | Desk, drawer, archive, warehouse | Use only to establish distance and capacity. | — | not yet used |
| Branch prediction | Choosing a route before reaching a fork | Explain recovery cost after the analogy. | — | not yet used |
| False sharing | Two people repeatedly erasing separate fields on one shared whiteboard | Emphasize cache-line granularity. | — | not yet used |
| Memory bandwidth | Lanes carrying bytes per second | Distinguish lane capacity from trip latency. | — | not yet used |
| CPU affinity | Assigning a worker to a workstation | Explain lost flexibility. | — | not yet used |
| NUMA | Multiple workshops with local storerooms connected by a corridor | Keep CPU placement and memory placement distinct. | — | not yet used |
| eBPF | Temporary sensors attached to defined points in a running machine | Sensors are constrained programs, not passive magic. | — | not yet used |
| Benchmark | Controlled scientific experiment | Use literally: hypothesis, controls, repetitions, uncertainty. | Chapters 2, 4, 5 | used |

## Notes on current use

- **Latency vs throughput** and **Saturation** are both used in Chapter 3,
  built directly on top of the blueprint's own supermarket-checkout core
  example for that chapter.
- **Benchmark as controlled scientific experiment** is used literally
  starting in Chapter 2 (defining a workload and metric before measuring),
  reinforced in Chapter 4 (benchmarking hygiene) and Chapter 5 (the
  investigation loop's hypothesis/falsification framing).
- No new analogies were introduced in Chapters 1–5 beyond the seed list;
  Chapter 1's incident narrative is told directly, without a standing
  analogy, since the blueprint's Section 17 list has no entry for
  "on-CPU/off-CPU time accounting" yet.
