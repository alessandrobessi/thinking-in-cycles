# Roadmap

Drafting order follows BLUEPRINT.md Section 24 ("Draft by concept
dependency, not by excitement about tools"). This file tracks actual
progress against that order.

## Phase 1 — Foundation (complete)

- [x] Chapters 1-5 (Part I — Measuring Reality)
- [x] Benchmark harness (`cyclelab compute` mode; `labs/scripts/ch1-ch5_*.sh`)
- [x] Environment doctor (`scripts/doctor.sh`)
- [x] Glossary and misconception registry, seeded for Chapters 1-5
- [x] `cyclelab branch` mode (built in Phase 2, for Chapter 9)
- [ ] `cyclelab` modes beyond `compute`/`branch` (sequential-memory,
      random-memory, bandwidth, false-sharing, lock-contention, syscall,
      sleep, numa, mixed) — currently recognized-but-stubbed
- [ ] `labs/mini-service` (second recurring example) — not started; see
      `labs/mini-service/README.md`

## Phase 2 — CPU model (complete)

- [x] Chapters 6-10 (Part II — What the CPU Is Doing)
- [x] `cyclelab compute --chains=N` (Chapters 7-8) and `cyclelab branch`
      (Chapter 9) modes, exercised together with `cyclelab compute`
- [x] Architecture-review pass (x86-64 / Arm64) — see
      `book/part-2-what-the-cpu-is-doing/README.md`'s note on portability
      and the architecture-neutral prose / arch-specific-sidebar split
      applied throughout Chapters 6-10
- Note: this book's reference machine is macOS/Arm64, so `perf`-based
  commands in Chapters 7, 9, and 10 are documented against `perf`'s
  stable interface but not tested against real captured output; each is
  clearly marked schematic, with a portable, tested fallback lab
  covering the same underlying phenomenon.
- [x] Guided-lab scripts for Chapters 6-9 (`labs/scripts/ch6_build_and_disassemble.sh`,
      `ch7_ipc_intuition.sh`, `ch8_dependency_chains.sh`, `ch9_branch_prediction.sh`);
      Chapter 10 has no dedicated script (its lab is a direct `perf stat`
      invocation, documented in the chapter text)

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
  Parts I-II; `scripts/prepare_manuscript_for_publish.py` is a stub)
- CI wiring for any of the validators above
- Figures (`figures/source/`, `figures/generated/` are empty by design —
  see their READMEs)
