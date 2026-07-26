# Style Guide

This file is the standalone source of truth for day-to-day writing
conventions on this project.

## Voice

- Write to a capable engineer, not to a novice child and not to a CPU architect.
- Use precise, concrete sentences.
- Introduce the plain-language mechanism before the technical term.
- Use "may," "can," and "on this system" when hardware support varies.
- Distinguish facts, hypotheses, and heuristics.
- Never call a result "obvious" before the reader has the model.
- Never shame the reader for having trusted a common metric.
- Avoid performance folklore and macho optimization language.

## Commands

- Commands must be copyable.
- Mark placeholders clearly.
- Explain required privileges.
- Explain process-wide, thread-wide, CPU-wide, and system-wide scope.
- Include cleanup steps.
- Avoid pipelines that silently discard errors.
- Show how to save raw data.
- Prefer commands that are available in upstream tools.
- State when output differs by CPU, kernel, or distribution.

## Numbers

- Use measured numbers only with environment provenance.
- Use rounded illustrative numbers when teaching a concept.
- Never compare percentages whose denominators differ.
- Never claim a universal threshold for "good IPC," "bad miss rate," or "high context-switch count."
- Use distributions for latency and repeated benchmarks.

## Architecture portability

The main prose should remain architecture-neutral. Put x86, Arm64, Intel,
AMD, and vendor-specific details in:

- sidebars;
- lab notes;
- tool-availability boxes;
- appendices.

The manuscript must not use one Intel event name as though it were a
universal CPU concept.

## Related documents

- `templates/chapter-template.md` — structural template every chapter follows.
- `templates/lab-template.md` — structural template every guided lab follows.
- `misconceptions.md`, `analogy-registry.md`, `glossary.md` — registries to
  update whenever a chapter introduces a new instance of any of these.
