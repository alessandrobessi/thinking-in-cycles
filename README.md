# Thinking in Cycles

### A Mental Model for Linux Performance

*Thinking in Cycles* is a practical, intuition-first guide to Linux
performance engineering. It builds a cumulative model of how workloads
become processes and threads, how Linux schedules them, how CPUs execute
instructions, how caches and memory shape performance, how topology and
NUMA affect placement, and how tools such as `perf`, flame graphs, BCC,
and `bpftrace` reveal different parts of the system. Rather than teaching
a collection of commands, it teaches readers to define a workload, form a
hypothesis, select the least invasive measurement, interpret the
evidence, and prove whether an optimization actually helped.

See [`BLUEPRINT.md`](BLUEPRINT.md) for the full founding design document
this project follows — mission, teaching philosophy, chapter-by-chapter
outline, concept dependency graph, style guide, and every other
editorial decision. This README covers what's actually implemented.

## Status

**The book's manuscript is complete: all thirty chapters (Parts I
through VI) plus Appendices A-G are drafted.**
`cyclelab`'s `compute`, `branch`, `sequential-memory`, `random-memory`,
`bandwidth`, `false-sharing`, `lock-contention`, and `sleep` modes are
implemented; its other three modes (`syscall`, `numa`, `mixed`) are
recognized by the CLI but not yet built. Every mode reports process-wide
voluntary/involuntary context-switch counts (POSIX `getrusage`, portable
to Linux and macOS), used throughout Part V and VI's scheduling and
off-CPU chapters. Part III adds a small, dependency-free flame-graph
toolchain (`labs/scripts/foldstacks.py`, `flamegraph_svg.py`,
`capture_sample_profile.sh`) built on macOS's `sample`(1) utility,
reused in Part VI to capture real, tested off-CPU (blocked-in-mutex)
stacks — something Linux's on-CPU-only `perf record` default cannot do
without dedicated off-CPU tooling. The manuscript renders cleanly to
HTML, PDF, and EPUB (`python3 scripts/prepare_manuscript_for_publish.py`),
and `.github/workflows/ci.yml` is ready to run on push once this repo
has a git remote. See [`ROADMAP.md`](ROADMAP.md) for
the full phase-by-phase status.

## Quickstart

```bash
make doctor          # report this machine's lab-environment capabilities
make lab-cyclelab    # build cyclelab (debug + release)
make smoke           # build cyclelab and run a minimal functional check
```

Then read [`book/README.md`](book/README.md), starting with
[Part I — Measuring Reality](book/part-1-measuring-reality/README.md).

## Repository map

```text
BLUEPRINT.md              founding design document (source of truth)
ROADMAP.md                phase-by-phase progress against BLUEPRINT.md Section 24
CONTRIBUTING.md           how to add a chapter, update registries, contribute code
style-guide.md            voice, commands, numbers, architecture portability
glossary.md               every term introduced so far, by concept level
misconceptions.md         the misconception registry (M01-M43)
analogy-registry.md       canonical analogies and where they're used
concept-graph.yaml/.md    machine- and human-readable concept dependency graph
_quarto.yml, index.md     Quarto book config and title page (repo root -- see below)
book/                     the manuscript itself, one directory per Part
labs/cyclelab/            the recurring C lab tool (see its own README)
labs/scripts/             helper scripts backing each chapter's guided lab
labs/mini-service/        planned second recurring example (not built yet)
figures/                  diagram sources and generated assets (mostly empty; one real flame graph)
references/               bibliography and per-chapter reference stubs
templates/                chapter, lab, and performance-report templates
scripts/                  doctor.sh, three validators, smoke test, and the Quarto publish script (all real)
publish/_book/            rendered HTML/PDF/EPUB output (gitignored, not committed)
```

`_quarto.yml` lives at the repo root, not in `publish/`, because Quarto
book projects cannot render chapter content reached via `../`
parent-directory paths (confirmed directly — see `ROADMAP.md`'s Quarto
entry). `publish/` still holds the actual build output
(`project.output-dir: publish/_book`), matching BLUEPRINT.md Section
20's intent that it be "where the built book lives."

```bash
python3 scripts/prepare_manuscript_for_publish.py   # renders HTML + PDF + EPUB into publish/_book/
```

## License

Code (`labs/`, `scripts/`) is MIT licensed. Book prose (`book/`) is
copyright, all rights reserved, pending a publication decision. See
[`LICENSE`](LICENSE) for the full split.
