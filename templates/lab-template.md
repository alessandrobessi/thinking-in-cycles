# Lab: [Lab Title]

**Related chapter:** [Chapter N — Title]
**Portability:** [portable | hardware-dependent | privileged | bare-metal recommended]
**Estimated time:** [X minutes]
**Requirements:** [tools / binaries / permissions needed]

This template follows BLUEPRINT.md Section 9 ("Guided Lab") and Section 14
(Benchmarking Hygiene Checklist). A lab is complete only when a reader with
none of the described hardware or permissions can still follow the Fallback
Path and learn something true.

## Objective

What question this lab lets the reader answer for themselves.

## Hygiene Checklist

Pick the items from Section 14 that apply to this lab; delete the rest.

**Define**
- [ ] Exact workload being measured
- [ ] Primary metric
- [ ] Cold vs. warm question

**Control**
- [ ] Fixed software versions / build flags
- [ ] Setup separated from measured work
- [ ] Background load controlled where possible
- [ ] Dead-code elimination prevented and output validated

**Repeat**
- [ ] Enough repetitions to see a distribution
- [ ] Interleaved or randomized variants where comparing two things

## Setup

Commands or state required before the timed command runs.

## Command(s)

```bash
# copyable, with placeholders clearly marked, e.g. <THREADS>
```

## Expected Qualitative Result

Describe the *direction* or *shape* of the result (e.g., "throughput should
rise then flatten," "the two distributions should mostly overlap"). Never a
specific number — Section 9 forbids requiring the reader to reproduce an
exact numeric value.

## Interpretation

What the result does and does not prove.

## Common Failure Modes

What tends to go wrong (missing tool, insufficient permissions, noisy
machine, misread output) and how to recognize it.

## Fallback Path

What the reader can still do and learn if the primary hardware, tool, or
permission is unavailable. Required by Section 25's Definition of Done for a
Chapter.

## Cleanup

Anything started in Setup that should be stopped or removed.

## Raw Data / Reproduction Notes

Where raw output was captured (if any) and the environment provenance
(Section 7.5: CPU, kernel, compiler/flags, thread count, placement) needed to
interpret it.
