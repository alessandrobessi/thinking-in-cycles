# Roadmap

Drafting order follows BLUEPRINT.md Section 24 ("Draft by concept
dependency, not by excitement about tools"). This file tracks actual
progress against that order.

## Phase 1 — Foundation (in progress)

- [x] Chapters 1-5 (Part I — Measuring Reality)
- [x] Benchmark harness (`cyclelab compute` mode; `labs/scripts/ch1-ch5_*.sh`)
- [x] Environment doctor (`scripts/doctor.sh`)
- [x] Glossary and misconception registry, seeded for Chapters 1-5
- [ ] `cyclelab` modes beyond `compute` (branch, sequential-memory,
      random-memory, bandwidth, false-sharing, lock-contention, syscall,
      sleep, numa, mixed) — currently recognized-but-stubbed
- [ ] `labs/mini-service` (second recurring example) — not started; see
      `labs/mini-service/README.md`

## Phase 2 — CPU model (not started)

- [ ] Chapters 6-10
- [ ] `cyclelab compute` and `branch` modes exercised together
- [ ] Architecture-review pass (x86-64 / Arm64)

## Phase 3 — Profiling (not started)

- [ ] Chapters 11-15
- [ ] Symbol and unwinding fixtures
- [ ] Flame-graph generation pipeline

## Phase 4 — Memory (not started)

- [ ] Chapters 16-20
- [ ] `sequential-memory`, `random-memory`, `bandwidth`, `false-sharing` modes
- [ ] Multi-architecture event audit

## Phase 5 — Topology (not started)

- [ ] Chapters 21-25
- [ ] Affinity and NUMA labs
- [ ] Single-node fallback material

## Phase 6 — Tracing and synthesis (not started)

- [ ] Chapters 26-30
- [ ] BCC and `bpftrace` labs
- [ ] Full service case study (Chapter 30)
- [ ] Production-safety review

## Phase 7 — Editorial integration (not started)

- [ ] Dependency validation (`scripts/validate_concept_graph.py` — currently a stub)
- [ ] Chapter metadata validation (`scripts/validate_chapter_metadata.py` — currently a stub)
- [ ] Link validation (`scripts/validate_links.py` — currently a stub)
- [ ] Analogy consistency pass
- [ ] Command testing across all chapters
- [ ] Bibliography completion
- [ ] Technical review (perf practitioner, eBPF practitioner, CPU/memory
      specialist, audience-representative engineer)

## Not yet started, no matter the phase

- Quarto HTML/PDF/EPUB build (`publish/_quarto.yml` exists and lists only
  Part I; `scripts/prepare_manuscript_for_publish.py` is a stub)
- CI wiring for any of the validators above
- Figures (`figures/source/`, `figures/generated/` are empty by design —
  see their READMEs)
