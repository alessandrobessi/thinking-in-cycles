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
- [x] `cyclelab sequential-memory`/`random-memory`/`bandwidth`/`false-sharing`
      modes (built in Phase 4, for Chapters 16-19)
- [ ] `cyclelab` modes beyond those six (lock-contention, syscall,
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

## Phase 3 — Profiling (complete)

- [x] Chapters 11-15 (Part III — Where the CPU Time Goes)
- [x] Symbol and unwinding fixtures (Chapter 13's three-variant build:
      full debug info, no debug info, frame-pointer-omitted)
- [x] Flame-graph generation pipeline — built from scratch, since this
      book's macOS reference machine has no `perf`/FlameGraph Perl
      toolchain: `labs/scripts/foldstacks.py` (macOS `sample` → folded
      stacks, same format as `stackcollapse-perf.pl`),
      `labs/scripts/flamegraph_svg.py` (folded stacks → SVG, with a
      differential `--diff-against` mode), and
      `labs/scripts/capture_sample_profile.sh` (reusable capture+fold
      wrapper). One real captured figure,
      `figures/generated/ch14-flame-graph-example.svg`, is used in
      Chapter 14 — a deliberate, narrow exception to the "no figures
      yet" policy, since that chapter is specifically about reading a
      visual artifact.
- Note: as in Phase 2, this book's reference machine is macOS, so all of
  Part III's guided labs use macOS `sample` as the tested, portable
  primary tool; every `perf record`/`perf report`/`perf annotate`/`perf
  diff` command shown is documented against `perf`'s stable interface
  but not tested against real captured output, clearly marked as such.
- [x] Guided-lab scripts for Chapters 11-15
      (`ch12_profile_hot_path.sh`, `ch13_symbol_availability.sh`,
      `ch15_before_after.sh`; Chapters 11 and 14 reuse
      `capture_sample_profile.sh` directly rather than a per-chapter
      script)

## Phase 4 — Memory (complete)

- [x] Chapters 16-20 (Part IV — Why Memory Changes Everything)
- [x] `sequential-memory`, `random-memory`, `bandwidth`, `false-sharing`
      modes — `sequential-memory`/`random-memory` share one
      pointer-chase implementation (Sattolo's-algorithm single-cycle
      permutation for the random case); `bandwidth` is a
      prefetch-friendly streaming-sum kernel deliberately different in
      shape from the pointer chase; `false-sharing` compares packed vs.
      cache-line-padded per-thread counters
- Note: as in Phases 2-3, every number in Chapters 16-20 is real,
  captured data from this book's reference machine (Apple M4, macOS,
  arm64) — cache-hierarchy latency cliffs, stride effects, false-sharing
  scaling, and bandwidth saturation curves. Chapter 20's `perf
  mem`/`perf c2c`/PCM commands are documented against each tool's stable
  interface but not tested, clearly marked as such; its portable lab
  instead synthesizes Chapters 16-19's own tested results as a "rung 1"
  tool-ladder exercise.
- [ ] Formal multi-architecture (x86-64 vs. Arm64) event/timing audit —
      not done; this Part follows the established architecture-neutral
      prose policy (numbers are captioned with machine/architecture
      provenance, no architecture-specific claim is stated as
      universal) but all real measurements were taken on one machine
      (Arm64), not cross-checked against x86-64 hardware
- [x] Guided-lab scripts for Chapters 16-19
      (`ch16_memory_hierarchy.sh`, `ch17_stride_sweep.sh`,
      `ch18_false_sharing.sh`, `ch19_bandwidth_scaling.sh`); Chapter 20
      has no dedicated script (its lab is a written synthesis exercise
      over Chapters 16-19's existing results, documented in the chapter
      text)

## Phase 5 — Topology (complete)

- [x] Chapters 21-25 (Part V — Where the Work Runs)
- [x] Cross-cutting `cyclelab` addition: every mode now reports
      process-wide voluntary/involuntary context-switch counts via
      POSIX `getrusage(2)` (`labs/cyclelab/src/rusage_util.c`) — portable
      to Linux and macOS, unlike `RUSAGE_THREAD`. Real, reproducible
      scheduling-pressure (Chapter 21) and noisy-neighbor interference
      (Chapter 22) data came directly from this, without needing `perf
      sched`/`pidstat`.
- [x] Affinity labs: **not testable** on this book's reference machine —
      macOS has no user-accessible hard-affinity API, confirmed directly
      by `cyclelab`'s own `--affinity` flag (reports unsupported and
      continues). Chapter 23's lab uses that honest result plus natural
      unpinned-variance measurement as its portable content; Linux
      `taskset`/`sched_setaffinity` experiments are documented, not
      tested.
- [x] Single-node fallback material: this reference machine (Apple M4)
      has no NUMA topology at all, confirmed by `scripts/doctor.sh`'s
      own NUMA check. Chapters 24-25 follow BLUEPRINT.md Section 13.2's
      explicit allowance for schematic multi-socket data on single-node
      hardware; Chapter 25 in particular has no measured lab at all,
      only a structured prediction/reasoning exercise in its place.
- [x] Guided-lab scripts for Chapters 21-23
      (`ch21_runnable_pressure.sh`, `ch22_noisy_neighbor.sh`,
      `ch23_affinity_availability.sh`); Chapters 24-25 have no dedicated
      scripts (their labs use `scripts/doctor.sh` directly and a
      written reasoning exercise, respectively, documented in the
      chapter text)

## Phase 6 — Tracing and synthesis (complete)

- [x] Chapters 26-30 (Part VI — Seeing the Invisible) — **the full
      30-chapter manuscript is now drafted.**
- [x] Cross-cutting `cyclelab` additions: `lock-contention` mode
      (threads contending one shared `pthread_mutex`, a genuinely
      serializing, blocking workload) and `sleep` mode (threads that
      intentionally `nanosleep()`) — both real, tested, built to give
      Chapter 29's off-CPU lab something to actually measure.
- [x] `scripts/doctor.sh` addition: a "dtrace" section, reporting this
      SIP-enabled macOS reference machine's real, tested finding —
      `dtrace` is present but refuses to list probes without elevated
      privileges (`DTrace requires additional privileges`) — direct
      evidence for Chapter 26's "dynamic tracing requires privilege"
      caution, not a hypothetical.
- [ ] BCC and `bpftrace` labs — **not testable** on this reference
      machine: both are Linux-only and confirmed absent by
      `scripts/doctor.sh`'s own "bpftrace / BCC" check. Chapters 26-28's
      Linux commands are documented against each tool's stable
      interface, not tested, following the same honest pattern already
      used for `perf` throughout Parts II-IV. Chapter 28's guided lab
      substitutes a real, tested, hand-rolled aggregation (count-grouped
      -by-key and histogram, computed from `cyclelab`'s own JSON output)
      demonstrating the same underlying concepts `bpftrace`/BCC compute
      in-kernel.
- [x] Full service case study (Chapter 30): a real, tested 8-step
      investigation built from `cyclelab lock-contention` (Steps 1-4,
      revealing serialization and lock waiting) and `cyclelab bandwidth`
      (Step 5, the bottleneck moving to memory bandwidth after the lock
      is removed) — Steps 6-7 (NUMA thread/memory placement) are a
      documented, schematic continuation, since this reference machine
      has no NUMA topology (Chapter 24).
- [ ] Production-safety review — not applicable in the sense BLUEPRINT.md
      intends (a review by a practitioner with production eBPF
      experience); flagged for Phase 7's technical review instead.
- Note: Chapter 29's off-CPU lab produced this phase's most genuinely
  surprising real finding: macOS's `sample`(1) captures every thread's
  stack on a wall-clock interval regardless of run state (unlike Linux
  `perf record`'s on-CPU-only default), so a `lock-contention` capture
  under heavy contention showed **77.8%** of sampled frames genuinely
  blocked inside a mutex wait, versus **0.0%** for an equivalent
  `compute`-mode capture — real, dramatic, reproducible off-CPU
  evidence without any Linux-only tracing infrastructure. Separately,
  this reference machine's `getrusage` was confirmed (via a minimal
  standalone test, not just `cyclelab`) to report `ru_nvcsw`
  (voluntary context switches) as exactly `0` in every mode and every
  configuration tested, including purely intentional `nanosleep()`
  calls — a genuine Darwin platform limitation, documented honestly in
  `labs/cyclelab/README.md` and in Chapter 29 rather than papering over
  it with invented numbers.
- [x] Guided-lab scripts for Chapters 26, 28, 29, and 30
      (`ch26_probe_availability.sh`, `ch28_manual_aggregation.sh`,
      `ch29_offcpu_lock_contention.sh`, `ch30_investigation_case_study.sh`);
      Chapter 27 has no dedicated script (its lab is a written
      verifier-prediction reasoning exercise, documented in the chapter
      text, consistent with the Chapter 20/25 pattern).

## Phase 7 — Editorial integration (mostly complete; technical review outstanding)

- [x] Dependency validation (`scripts/validate_concept_graph.py`) — real,
      implemented. Checks: every chapter's `**Prerequisites:**` line cites
      only earlier chapters; the 30-question narrative chain matches
      verbatim end to end; every concept-graph.yaml term with a non-null
      `introduced_in_chapter` agrees with glossary.md's own tag; every
      `M<N>` misconception reference in any chapter resolves to a real
      heading in `misconceptions.md`, which itself has no duplicate or
      gapped ID. Run against the finished book: **0 errors**, 1 warning
      (Chapter 16's Prerequisites line cites "Part II"/"Part III" instead
      of "(Chapter N)" — accurate content, just a different citation
      style, not a defect).
- [x] Chapter metadata validation (`scripts/validate_chapter_metadata.py`)
      — real, implemented. Checks: filename numbering matches each
      chapter's own H1; each `part-N-*` directory holds exactly its five
      contiguous chapters in order; exactly one Opening Question, Guided
      Lab, Key Takeaway, and Next Obvious Question section per chapter;
      the Opening Question is a single sentence ending in "?"; the Guided
      Lab states a portability tag and a fallback path; image references
      have non-empty alt text. Found and fixed two real, pre-existing
      Definition-of-Done gaps: Chapter 1's Guided Lab never labeled its
      already-present fallback path with the standard `**Fallback
      path:**` tag (added), and Chapter 25 referenced misconception M10
      without the required `## Common Misconceptions` section existing
      at all (added, covering M10 and M11 revisited). Final run: **0
      errors**, 7 warnings (chapters that fold "Worked Example" into
      their Incident/Story section, or use blueprint-specified
      alternative headings like Chapter 28's "Tool choice policy" in
      place of "Tool View" — legitimate, pre-existing structural
      choices, not defects).
- [x] Link validation (`scripts/validate_links.py`) — real, implemented.
      Checks every relative Markdown link and every repo-relative path
      mentioned anywhere across `book/`, `templates/`, `references/`,
      and the top-level registries. Verified against a deliberately
      broken link (confirmed it's caught) before trusting a clean run.
      Final run: **0 errors** across 78 files.
- [x] Shell syntax check across every chapter's fenced `bash`/`sh` code
      block (46 total) plus all 26 `labs/scripts/*.sh`/`scripts/*.sh`
      files, via `bash -n`. Found and fixed one real issue: Chapter 25
      had a literal `numastat -p <pid>` inside a fenced code block,
      where the unescaped `<` is invalid shell redirection syntax —
      changed to `numastat -p "$PID"` with an inline comment. Final
      sweep: **0 syntax errors**.
- [x] Analogy consistency pass — audited all thirteen seed analogies
      directly against their citing chapter's actual text (not just
      against this registry's own prior claims). Found two real gaps:
      Chapter 3's registry entry claimed the "Latency vs throughput"
      seed analogy (vehicles/roads) was used, but the chapter's Worked
      Example only ever used the checkout-line analogy (which does cover
      the same ground) — corrected the registry to record this honestly
      as **substituted** per BLUEPRINT.md Section 17's own "retired when
      they stop helping" allowance, rather than leave a false "used"
      claim. Chapter 2's registry entry claimed the "Benchmark as
      controlled scientific experiment" analogy started there, but the
      chapter never actually used the words "experiment"/"hypothesis" —
      fixed by adding two sentences to Chapter 2's Practical Implications
      that state this directly, tying its "define the operation and
      metric first" content to that standing analogy's first step.
- [x] Bibliography completion — cross-checked every chapter's Further
      Reading URL against `references/bibliography.md` (0 chapters cite
      a URL absent from the bibliography); checked for duplicate URLs
      within the bibliography itself (none, 26/26 unique); confirmed all
      30 per-chapter reference stubs exist.
- [ ] Technical review (perf practitioner, eBPF practitioner, CPU/memory
      specialist, audience-representative engineer) — **cannot be
      completed autonomously.** This requires human domain-expert
      reviewers reading the finished manuscript; nothing above
      substitutes for it. Remains genuinely outstanding.
- Note: `python3 scripts/validate_concept_graph.py`,
  `validate_chapter_metadata.py`, and `validate_links.py` are now real
  tools, not stubs — run all three via `make validate`.

## Appendices A-G (complete)

Not part of BLUEPRINT.md Section 24's seven drafting phases (which
cover only the 30 numbered chapters), but requested and completed
alongside CI wiring and the Quarto render below.

- [x] All seven appendices drafted as reference material (BLUEPRINT.md
      Section 22's own framing: bullet-scoped, no chapter template,
      cross-referencing the chapters they extend rather than repeating
      them): Appendix A (trustworthy lab machine), B (`perf` command
      map), C (hardware event portability), D (runtime-specific
      profiling notes), E (containers and cgroups), F (statistical
      reasoning for benchmarks), G (production safety).
- [x] Appendix F is genuinely real, not schematic: built around two
      fresh `cyclelab compute` comparisons (twelve repetitions each,
      captured for this appendix specifically) — a real, large effect
      (`--chains=1` vs. `--chains=2`, Cohen's *d* ≈ 1.96, non-overlapping
      bootstrap confidence intervals) contrasted with a real "no
      effect" case (the same config run as two separate sessions,
      heavily overlapping intervals) — giving Chapter 4's own
      benchmarking-hygiene vocabulary (medians, confidence intervals,
      effect size) concrete numbers instead of illustrative
      placeholders.
- [x] Appendix C's hybrid-CPU section uses this reference machine's own
      real topology (`sysctl -n hw.perflevel0.physicalcpu` /
      `hw.perflevel1.physicalcpu`: 4 performance + 6 efficiency cores)
      as real, confirmed data, not a hypothetical example.
- [x] All three validators re-run clean against the appendices:
      `validate_links.py` now checks 85 files (was 78), still 0 errors.
      `validate_concept_graph.py`/`validate_chapter_metadata.py` don't
      scan appendices (they target `book/part-*/chapter-*.md`
      specifically, since appendices aren't chapters and have no
      Prerequisites/New-concepts/Opening-Question structure to check).

## Quarto HTML/PDF/EPUB build (complete)

BLUEPRINT.md Section 26's "Definition of Done for the Book" requirement
("the HTML, PDF, and EPUB builds succeed") is met: all three formats
render cleanly from the full 39-document manuscript (30 chapters + 7
appendices + preface + index).

- [x] Installed Quarto 1.10.18 **without root** via its official macOS
      tarball (`quarto-1.10.18-macos.tar.gz`, extracted to
      `~/.local/quarto`) after the Homebrew cask's `.pkg` installer
      failed non-interactively (it shells out to `sudo`, which needs a
      real terminal for a password prompt this environment doesn't
      have). No LaTeX/TinyTeX install was needed: PDF renders via
      Quarto's bundled Typst compiler instead (`format: typst`),
      confirmed present by `quarto check`.
- [x] Found and fixed a real, non-obvious structural bug: `_quarto.yml`
      originally lived in `publish/` with chapter paths like
      `../book/part-1-.../chapter-01-....md`. Quarto's own render log
      showed every one of the 39 documents being "processed," and exit
      cleanly with no error -- but the actual output directory
      contained only `index.html`, with every chapter's real content
      silently missing. Root-caused via a minimal isolated
      reproduction (a two-file test book project): Quarto book
      projects do not correctly render chapter content reached via
      `../` parent-directory paths, even though they don't error on
      it. Fixed by moving `_quarto.yml` (and the book's title page,
      `index.md`) to the repository root, with `project:
      output-dir: publish/_book` keeping the actual build output inside
      `publish/` per BLUEPRINT.md Section 20's intent. Documented in
      `_quarto.yml`'s own header comment so this doesn't get
      "helpfully" moved back.
- [x] Implemented `scripts/prepare_manuscript_for_publish.py` for real:
      confirms every chapter path in `_quarto.yml` exists, runs the
      three Phase 7 validators and refuses to render if any reports an
      error (`--skip-validation` available for local iteration only),
      copies any new file from `figures/source/` into
      `figures/generated/` (a no-op today -- `figures/source/` has no
      editable sources yet -- but real once it does), then invokes
      `quarto render`.
- [x] Verified real output, not just a clean exit code: HTML render
      produces 58 files (one page per chapter/appendix plus search
      index, confirmed to contain each chapter's actual real text, not
      a placeholder) with a working search index (473 entries); PDF
      renders to a valid 172-page, 3.9MB PDF 1.7 document via Typst;
      EPUB renders to a valid EPUB document via Pandoc. All three
      together: 8.6MB in `publish/_book/` (gitignored, not committed).
- Run it yourself: `python3 scripts/prepare_manuscript_for_publish.py`
  (needs `quarto` on `PATH`; add `--skip-validation` to skip the
  registry/link checks for a faster local iteration loop, or
  `--format html`/`--format typst`/`--format epub` for just one format).

## Not yet started, no matter the phase

- CI wiring: `.github/workflows/ci.yml` is authored and real (a
  `cyclelab` job matrixed across `ubuntu-latest`/`macos-latest` running
  `make doctor` + `make lab-cyclelab` + `make smoke`, and a `manuscript`
  job running `make validate`), following BLUEPRINT.md Section 21's "CI
  must never fail a build over a performance threshold" rule directly
  (every step is a compile, a correctness/schema check, or a manuscript
  consistency check, never a timing assertion). It cannot actually run
  yet: **this repository has no git remote configured**, so nothing is
  pushed anywhere GitHub Actions could trigger from. It will activate
  automatically the first time this repo is pushed to GitHub.
- Figures: still mostly empty by design (see `figures/source/` and
  `figures/generated/` READMEs) — the one exception is Chapter 14's real
  captured flame graph, added because that chapter is specifically about
  reading a visual artifact
