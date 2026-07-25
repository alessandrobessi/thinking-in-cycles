# Performance Report: [Title]

This template follows BLUEPRINT.md Section 30 ("Final report template"),
Section 14 (Benchmarking Hygiene Checklist), and Section 7.5 (measurement
provenance). A report using this template should let a skeptical reader
tell the difference between what was observed, what it means, and what is
recommended.

## Workload and Success Metric

What exact workload was measured, and what metric defines "better."

## Environment

- CPU model and topology
- Kernel version
- Compiler and flags
- Workload input
- Process / thread count
- CPU and memory placement
- Frequency / governor state (if relevant)
- Warm-up and repetition policy
- Measurement command(s)
- Known virtualization or permission constraints

## Baseline Distribution

Not a single number: the shape of repeated, controlled baseline runs.

## Initial Observations

What stood out before any hypothesis was formed.

## Hypotheses Considered

Each hypothesis, and whether it was supported, rejected, or left open.

## Measurements and Overhead

What was measured, with which tool, at what estimated overhead.

## Changes Made

Exactly one change per experiment; state what changed and what did not.

## Before/After Evidence

Controlled, interleaved-where-possible comparison. State whether profiles or
timings are normalized by time, requests, iterations, or another unit.

## Bottleneck Movement

Where time went before, and where it went after — including any new
bottleneck introduced by the fix.

## Limitations

What this report does not prove: untested inputs, untested scales, single
machine, single day, etc.

## Rollback / Safety Considerations

How to undo the change, and what risk it carries if wrong.

## Next Experiment

The next hypothesis this result motivates.
