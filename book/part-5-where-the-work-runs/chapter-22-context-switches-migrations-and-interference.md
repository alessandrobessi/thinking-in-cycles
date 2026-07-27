# Context Switches, Migrations, and Interference

**Part:** Part V — Where the Work Runs
**Concept level:** 1
**Prerequisites:** runnable, running, sleeping, run queue (Chapter 21)
**New concepts:** voluntary context switch, involuntary context switch, migration, cache warmth, interrupt, steal time, noisy neighbor, CPU pressure

## Opening Question

How do migrations and neighboring workloads add noise and delay?

## Incident or Real-World Story

A team's benchmark results start drifting from run to run in a way
nobody can explain from the code alone — the same commit, run twice in a
row on the same machine, sometimes lands within a percent of a prior
baseline and sometimes 30% slower. Eventually someone notices the slow
runs cluster around times when a colleague's build job, or a backup
process, or another team's service happens to be running on the same
shared machine. Nothing about the benchmarked code changed between runs.
What changed was who else the CPUs were serving at the same time — a
**noisy neighbor**, competing for the exact same cores, at moments the
benchmark's own logs never recorded and its own metrics never
mentioned.

This is the multi-tenant version of Chapter 21's oversubscription
problem: instead of the workload's own threads outnumbering its CPUs,
someone else's threads are doing it, invisibly, from outside the
process being measured.

## Predict Before Measuring

Before reading further: if you benchmark the same workload twice —
once with the machine otherwise idle, once while a competing,
unrelated process runs full-tilt on the same cores — do you expect the
benchmark's own reported metrics (like context-switch counts) to show
any evidence of that competition, or would you need to inspect the
competing process directly to know it was there?

## Worked Example

Four real shapes of this problem, one of which this chapter's lab
reproduces directly: a thread that wakes frequently, gets migrated
between CPUs, and loses **cache warmth** — the accumulated benefit of
its data already sitting in a specific core's cache, which a
**migration** to a different core discards, forcing that data to be
refetched from further away (Chapter 16's hierarchy, paid again). A
benchmark sharing a core with unrelated background work, exactly this
chapter's lab. A virtual machine losing physical CPU time to its
hypervisor or to other tenants — **steal time**, a metric specific to
virtualized environments that directly quantifies this. And
**interrupts** concentrated on one CPU, which can make that specific
core's threads pay a tax the rest of the machine never sees.

## Core Intuition

A **voluntary context switch** is a thread giving up the CPU on its own —
blocking on I/O, a lock, or a sleep. An **involuntary context switch**
is the scheduler preempting a still-runnable thread so another one can
run — Chapter 21's signal for runnable pressure, now understood as a
*cost* in its own right, not just a symptom. Every switch, voluntary or
involuntary, has a real price beyond the time it takes to switch:
whatever made the previous thread's execution fast — its data sitting
warm in cache, its branch predictor tuned to its own patterns — is at
least partly lost, paid for again when that thread eventually resumes.
A **migration** compounds this by moving a thread to a *different* core
entirely, discarding cache warmth more completely than a same-core
switch would. **CPU pressure** is the general sense of demand for CPU
time exceeding what's readily available, whether from a workload's own
threads (Chapter 21) or, as in this chapter, from unrelated neighbors.

## Technical Explanation

The mandatory caution this chapter insists on, and the reason a new
misconception is worth naming directly: **context-switch counts alone
do not diagnose scheduler overhead.** A high switch count from many
short, voluntary, intentional handoffs (a producer-consumer pipeline
working exactly as designed) is not the same problem as a high switch
count from involuntary preemption caused by an oversubscribed or
contested machine — same metric, opposite implications. What actually
matters is *whether switches delay critical work or destroy useful
locality*, not the raw count. This chapter's lab makes that concrete:
the same benchmark, same code, same thread count, shows dramatically
different throughput and switch counts purely based on who else was
running — the switches didn't change the code's correctness; they
changed how much of the CPU's time and cache warmth the benchmarked
threads actually got to keep.

## Tool View

- What is measured: throughput and involuntary context switches for the
  same `cyclelab compute` benchmark, run alone and then again while a
  competing `cyclelab compute` process shares the machine.
- What is not measured: which specific CPU each thread ran on, or
  whether migrations specifically (as opposed to same-core preemption)
  occurred — that needs `perf sched` or `ps`/`taskset`-level CPU
  affinity inspection (Linux), documented below.
- Required permissions: none for this chapter's lab.
- Likely overhead: negligible for the measurement; the "noisy neighbor"
  itself is the point, not overhead to minimize.
- Portability: works anywhere `cyclelab` runs (two processes, no special
  tooling). On Linux, richer views of the same phenomenon:

  ```bash
  pidstat -w 1                    # context-switch rate over time, per process
  perf stat -e context-switches,cpu-migrations -- \
    ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=2
  ```

  **Documented, not tested** on this book's macOS reference machine.
  Steal time specifically is visible via `vmstat`'s `st` column inside a
  VM guest, or cloud-provider-specific monitoring — not reproducible
  without an actual virtualized/multi-tenant environment, so this
  chapter's portable lab approximates the same underlying phenomenon
  (competition for shared CPUs) with two ordinary processes instead.
- Common failure mode: seeing a high context-switch count and assuming
  scheduler misbehavior, without checking whether those switches are
  voluntary (often benign or even by design) or involuntary under real
  contention (this chapter's actual concern).

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch22_noisy_neighbor.sh
```

This benchmarks `cyclelab compute --threads=2` three times alone, then
three more times while a separate, competing `cyclelab compute`
instance (using most of this machine's remaining cores) runs
concurrently.

**Expected qualitative result:** throughput should drop and involuntary
context switches should rise substantially once the neighbor is
running. One example run on the reference machine for this book (Apple
M4, macOS, arm64, 10 logical CPUs, neighbor using 8 threads) showed:

```text
              throughput_ops_s   nivcsw
alone         472,372,623        11
alone         433,001,010        9
alone         445,743,610        47
neighbor      222,488,410        503
neighbor      177,155,849        524
neighbor      184,370,129        493
```

Throughput dropped by roughly 55-60%; involuntary switches rose by
roughly 10-50x.

**Interpretation:** nothing about the benchmarked code changed between
the two conditions — only who else was competing for the same CPUs.
This is the portable, reproducible version of the "why did the same
benchmark get slower with no code change" mystery from this chapter's
opening story, with the cause made visible instead of hidden.

**Fallback path:** if `python3` isn't available, run the alone/neighbor
commands directly (the script prints them as it runs) and read
`results.throughput_ops_per_s` and `results.context_switches.involuntary`
from each run's raw JSON.

**Cleanup:** the script waits for its own background neighbor process to
finish before exiting; no manual cleanup needed.

## Common Misconceptions

### *"Context-switch counts alone diagnose scheduler overhead." (M19)*

**Why it's wrong:** The impact of switches depends on why they occur,
where the critical thread waits, and what locality is lost — not on the
raw count, which can be high for entirely benign, voluntary reasons or
low while still hiding serious involuntary contention.

**Correct intuition:** This chapter's own lab shows the *same* metric
(involuntary switches) meaning something completely different depending
on whether it's elevated because of genuine contention (the
noisy-neighbor condition) or not — the number alone, without that
context, doesn't tell you which.

**Analogy:** A phone that rings fifty times today could mean fifty
important calls or one wrong number calling back fifty times — the
count alone doesn't tell you whether the interruptions actually
mattered.

## Practical Implications

Before attributing benchmark noise to "the machine" vaguely, check
whether something else was actually running on it at the same time —
this chapter's lab is a direct, reproducible way to confirm that
competing, co-located work is a real, measurable cause of variance, not
a hand-wave. In shared or multi-tenant environments, treat noisy
neighbors as an expected source of variance to control for (Chapter 4's
discipline) rather than a rare anomaly.

## Key Takeaway

**Scheduling events matter when they delay critical work or destroy
useful locality; their counts alone do not prove harm.**

## What to Remember

- Voluntary switches (a thread giving up the CPU on its own) and
  involuntary switches (the scheduler preempting a runnable thread) are
  mechanically different and carry different implications.
- Every switch has a real cost beyond its own duration: accumulated
  cache warmth and predictor tuning are at least partly lost and paid
  for again on resumption.
- A migration to a different core discards cache warmth more completely
  than a same-core switch.
- Steal time is the virtualization-specific version of losing CPU time
  to something outside your own workload's control.
- A high context-switch count is not inherently bad, and a low one is
  not inherently good — what matters is whether switches delay critical
  work or destroy locality (M19).
- Noisy neighbors are a real, measurable, reproducible source of
  benchmark variance in any shared environment, not a rare edge case.

## Further Reading

- Linux perf manual pages (`perf sched`, `perf stat` context-switch and
  migration events): <https://man7.org/linux/man-pages/man1/perf.1.html>
- `pidstat`(1) manual page.

## The Next Obvious Question

When does CPU affinity help, hurt, or merely hide a problem?
