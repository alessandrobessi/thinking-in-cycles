# Contributing

This project follows BLUEPRINT.md closely. If anything here and
BLUEPRINT.md disagree, BLUEPRINT.md wins — update this file to match it,
not the other way around.

## Before writing a chapter

Read, in order: `templates/chapter-template.md` (the required structure),
`style-guide.md` (voice, commands, numbers, architecture portability), and
BLUEPRINT.md Section 25 ("Definition of Done for a Chapter"). A chapter
isn't done until it satisfies every item in that checklist, including:

- opening question answerable in one sentence;
- prerequisites respected (check `concept-graph.md` for what's already
  been introduced by an earlier chapter);
- the incident/story and the worked example are genuinely distinct;
- the reader predicts something before measuring;
- every command has actually been run and tested, or is clearly marked
  schematic;
- the guided lab has a portability tag (BLUEPRINT.md Section 13.2:
  portable / hardware-dependent / privileged / bare-metal recommended)
  and a fallback path for when the primary tool or hardware isn't
  available;
- expected lab outcomes are qualitative, never a specific number to
  reproduce;
- at least two misconceptions are addressed where relevant (see
  `misconceptions.md` — reuse an existing entry if one fits; propose a
  new one, clearly marked "proposed — pending review," if none does);
- the key takeaway is exactly one bolded sentence;
- "The Next Obvious Question" is the verbatim opening question of the
  chapter that follows (verify this against the next chapter, not just
  against BLUEPRINT.md's shorthand "Next question:" line, since the two
  can differ in wording).

## Updating the registries

Whenever a chapter introduces a new instance of one of these, update the
registry in the same change:

- **New glossary term** → add it to `glossary.md`, and to
  `concept-graph.yaml`/`concept-graph.md` if it's one of the Section 11
  concept-graph terms (set `introduced_in_chapter`); supplementary terms
  outside the concept graph go in glossary.md's "Supplementary
  vocabulary" section instead.
- **New or reused misconception** → add or update the entry in
  `misconceptions.md`, including its "Used in chapters" line.
- **New or reused analogy** → update `analogy-registry.md`'s "First used
  in" and "Status" columns; don't introduce a new analogy for a concept
  that already has one in the Section 17 seed table unless the existing
  one has stopped working (and say why in the registry).

## Code (labs/, scripts/)

- C code in `labs/cyclelab` targets C11, builds warning-clean under
  `-Wall -Wextra`, and must build on both Linux and macOS (the two
  platforms this project actually tests against). Unavailable
  platform-specific features (e.g. CPU affinity pinning) must be reported
  as a warning at runtime, never silently ignored or faked.
- Shell scripts target bash, avoid `set -e` in files that need to report
  partial failure gracefully (see `scripts/doctor.sh`), and should be
  runnable from any working directory (resolve their own script
  directory rather than assuming the caller's cwd).
- New `cyclelab` modes should follow `labs/cyclelab/README.md`'s existing
  CLI conventions and JSON output schema rather than inventing new ones.

## Source policy

Per BLUEPRINT.md Section 19: technical claims must be anchored in primary
sources (kernel docs, `perf`/BCC/`bpftrace`/libbpf upstream docs, vendor
optimization manuals, original papers). Blog posts may motivate an
incident or teaching idea but not stand alone as the source for a
technical claim. Every hardware-specific event name or claim must be
checked against the relevant architecture documentation and kept out of
the architecture-neutral main prose (sidebars, lab notes, and appendices
only — see `style-guide.md`'s "Architecture portability" section).

## Editorial invariants

These must remain true through every revision (BLUEPRINT.md Section 23)
— the ones most likely to be violated by a well-intentioned edit:

- every specialized tool appears only after the phenomenon it measures;
- every optimization is presented as conditional, never as folklore;
- a counter never becomes a verdict by itself;
- CPU affinity and memory affinity are never conflated;
- "faster" always means faster for a stated workload and metric.

## Commit and review expectations

Small, reviewable changes preferred over large ones. If a change touches
a chapter, its registry updates (glossary/misconceptions/analogy-registry
and, if applicable, `concept-graph.yaml`) belong in the same change, not a
follow-up. The three validators referenced above are implemented and
real: run them before opening a change that touches a chapter or a
registry.

```bash
python3 scripts/validate_concept_graph.py    # prerequisites, glossary sync, misconception IDs, narrative chain
python3 scripts/validate_chapter_metadata.py # filenames, part/chapter order, required sections, DoD checks
python3 scripts/validate_links.py            # internal links and referenced lab paths
```

Each exits 0 with no ERROR-level findings, non-zero otherwise; WARNING-
level findings are informational (documented, legitimate deviations, not
bugs) and don't fail the exit code. `.github/workflows/ci.yml` runs the
same three validators (as `make validate`) plus `make lab-cyclelab` and
`make smoke` on Linux and macOS on every push and pull request against
<https://github.com/alessandrobessi/thinking-in-cycles> — run
everything locally before opening a change anyway, since CI catching a
problem after the fact is a safety net, not a substitute for manual
review against the Definition of Done checklist.
