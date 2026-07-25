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

**Part I (Chapters 1-5) is drafted.** `cyclelab`'s `compute` mode is
implemented; its other ten modes are recognized by the CLI but not yet
built. See [`ROADMAP.md`](ROADMAP.md) for the full phase-by-phase status.

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
misconceptions.md         the misconception registry (M01-M23)
analogy-registry.md       canonical analogies and where they're used
concept-graph.yaml/.md    machine- and human-readable concept dependency graph
book/                     the manuscript itself, one directory per Part
labs/cyclelab/            the recurring C lab tool (see its own README)
labs/scripts/             helper scripts backing each chapter's guided lab
labs/mini-service/        planned second recurring example (not built yet)
figures/                  diagram sources and generated assets (empty for now)
references/               bibliography and per-chapter reference stubs
templates/                chapter, lab, and performance-report templates
scripts/                  doctor.sh (real) + validator stubs + smoke test (real)
publish/                  Quarto book scaffold (not yet rendered)
```

## License

Code (`labs/`, `scripts/`) is MIT licensed. Book prose (`book/`) is
copyright, all rights reserved, pending a publication decision. See
[`LICENSE`](LICENSE) for the full split.
