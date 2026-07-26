# Chapter 21 — The Scheduler, Run Queues, and CPU Time

**Part:** Part V — Where the Work Runs
**Concept level:** 1 (Linux Execution — deliberately returned to now that Parts II-IV have built the CPU and memory model these mechanics execute within)
**Prerequisites:** on-CPU/off-CPU time (Chapter 1); the investigation loop (Chapter 5); bandwidth saturation as a shape (Chapter 19)
**New concepts:** runnable, running, sleeping, run queue, time slice intuition, wake-up, load balancing, scheduling class as a boundary

## Opening Question

How does Linux decide where runnable work executes?

## Incident or Real-World Story

A team doubles the worker-thread count on a batch job, expecting
throughput to roughly double along with it — the job is embarrassingly
parallel, and the machine has plenty of cores, or so they believe.
Throughput barely moves. CPU utilization, already near 100% before the
change, stays near 100% after it. Nothing about the utilization graph
suggests anything is wrong; by that measure, the machine was already
"fully used" before doubling the threads, so getting no further
improvement seems, at first, unsurprising.

What the team hadn't checked was how many logical CPUs the machine
actually has, versus how many threads they'd just asked it to run
simultaneously. They'd gone well past that number. The extra threads
weren't making the CPUs work harder — the CPUs were already saturated —
they were creating a **run queue** of runnable-but-waiting threads, each
taking turns for a shrinking slice of time. Utilization stayed high
because the CPUs never sat idle. Throughput stopped scaling because
"CPU busy" and "this specific thread is the one running right now" are
different claims, and the gap between them is exactly where the extra
threads' time was going.

## Predict Before Measuring

Before reading further: for a CPU-bound workload, if you increase
thread count from below the machine's logical CPU count to well above
it, do you expect a metric like *involuntary context switches* (the
scheduler preempting a still-runnable thread to let another one run) to
stay flat, grow gradually, or grow sharply once thread count crosses
that boundary? Hold your prediction for this chapter's lab.

## Worked Example

Two configurations make the mechanism concrete: four CPU-bound threads
on two CPUs, versus two CPU-bound threads and two sleeping threads on
the same two CPUs. In the first case, four threads compete for two
CPUs — at any instant, two are **running**, two are **runnable** and
waiting their turn in the run queue, and the scheduler is continuously
deciding who runs next. In the second case, the two sleeping threads
aren't competing for anything most of the time — they're neither
running nor runnable, just **sleeping**, so the two CPU-bound threads
each effectively get a whole CPU to themselves, indistinguishable from
running alone. Both configurations have "four threads" as their
headline description. Only one of them has real runnable pressure.

## Core Intuition

A thread is, at any instant, exactly one of: **running** (actually
executing on a CPU right now), **runnable** (ready to run, waiting for
one to become available), or **sleeping** (blocked, waiting on something
other than a CPU — Chapter 1's off-CPU time). The **run queue** is the
set of runnable threads a CPU (or the scheduler managing several) has
waiting. **Time slice intuition**: a running thread doesn't run forever
uninterrupted — it gets a bounded slice of time before the scheduler
reconsiders who should run next, especially when others are waiting.
**Wake-up** is a sleeping thread becoming runnable again, in response to
whatever it was waiting for becoming available. **Load balancing** is
the scheduler's ongoing effort to keep runnable work spread reasonably
evenly across available CPUs rather than piling up on some while others
sit idle. **Scheduling class** is worth naming as a boundary concept
without full detail here: Linux supports multiple scheduling policies
(the default time-shared class most workloads use, plus real-time
classes with different guarantees) — which class a thread belongs to
affects how these mechanics apply to it, a distinction this book treats
as a boundary to be aware of, not a mechanism to master.

## Technical Explanation

The mandatory distinction this chapter insists on: **CPU utilization
does not directly show how long a runnable thread waited before
running.** A machine can report high utilization while individual
threads spend meaningful time sitting in the run queue, runnable but not
yet running — utilization counts CPU busy-ness in aggregate; it says
nothing about any *specific* thread's wait. This is the same shape as
Chapter 1's opening problem (low utilization hiding a real bottleneck)
from the opposite direction: here, utilization can look completely
healthy while a real, measurable cost — queueing delay for runnable
threads — is accumulating in plain sight, if you know to look at the
right signal.

That signal, portably, is the **involuntary context switch** count:
every time the scheduler preempts a still-runnable thread to let another
one run, that's one involuntary switch, and it costs nothing to observe
(Chapter 22 covers the *cost* of switches themselves; this chapter uses
the *count* purely as evidence of runnable pressure). A sharp rise in
involuntary switches once thread count exceeds logical CPU count is
direct, quantified evidence of exactly the run-queue competition
utilization alone cannot show.

## Tool View

- What is measured: throughput and involuntary context switches across
  a thread-count sweep that crosses this machine's logical CPU count,
  using `cyclelab compute`'s built-in `context_switches` reporting
  (`labs/cyclelab/README.md` — process-wide `getrusage(2)` counters,
  portable to both Linux and macOS).
- What is not measured: *which* thread waited, for how long, or on which
  CPU — that level of detail needs `perf sched` or similar (Linux-only),
  documented below.
- Required permissions: none for this chapter's lab.
- Likely overhead: negligible; `getrusage` reads kernel-maintained
  counters without perturbing the workload.
- Portability: works anywhere `cyclelab` runs. On Linux, richer views are
  available:

  ```bash
  pidstat -w 1                 # per-process context-switch rates over time
  perf sched record -- ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=20
  perf sched latency           # per-thread average and max scheduling delay
  ```

  **Documented, not tested** on this book's macOS reference machine.
- Common failure mode: reading "utilization is high" as "the machine is
  optimally used," without checking whether the threads actually running
  are the ones on the critical path, or whether others are queued behind
  them — this chapter's opening story exactly.

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch21_runnable_pressure.sh
```

This sweeps `cyclelab compute --threads` from 1 up to 4x this machine's
logical CPU count, tabulating throughput and involuntary context
switches together.

**Expected qualitative result:** throughput should scale while thread
count stays at or below the logical CPU count, then flatten; involuntary
context switches should keep climbing well past that same point. One
example run on the reference machine for this book (Apple M4, macOS,
arm64, 10 logical CPUs) showed:

```text
threads  throughput_ops_s   nivcsw
1        232,687,245        6
5        1,106,303,523      249
10       1,748,063,530      2,992
20       1,778,368,388      3,357
40       1,782,984,070      3,278
```

Throughput barely changed from 10 to 40 threads (1.75B to 1.78B), while
involuntary switches jumped roughly 12x from 5 to 10 threads and stayed
elevated well beyond it.

**Interpretation:** the flattening throughput alone tells you the CPUs
are saturated; it does not tell you the extra threads are actively
churning through the run queue rather than simply idling — the
involuntary-switch count is what distinguishes "saturated and calm"
from "saturated and contested," a distinction utilization alone cannot
make, exactly this chapter's mandatory point.

**Fallback path:** if `python3` isn't available, run the five
`cyclelab compute --threads=...` commands directly and read
`results.throughput_ops_per_s` and `results.context_switches.involuntary`
from each run's raw JSON.

**Cleanup:** none.

## Common Misconceptions

There is no blueprint-seeded misconception registry entry specific to
this chapter; a new one (M37, proposed) is worth naming directly, extending M02 (Chapter 1)
into scheduling territory specifically: **"High CPU utilization means
the machine is optimally scheduling its work."** This is wrong because
utilization is an aggregate busy-time measure that says nothing about
whether specific threads are waiting in the run queue behind others —
a machine can be fully utilized while accumulating real, avoidable
queueing delay for latency-sensitive work. The evidence that
distinguishes the two: this chapter's own lab — throughput (a rough
utilization proxy) stays flat from 10 to 40 threads while involuntary
switches nearly triple, showing real additional contention that
utilization alone never surfaces.

## Practical Implications

Before concluding a CPU-bound workload is running optimally because
utilization looks high, check whether thread count exceeds available
logical CPUs and whether involuntary context switches are elevated. A
workload that "has plenty of CPU" by the utilization graph can still be
leaving real throughput on the table if it's oversubscribed relative to
available cores, exactly this chapter's opening story.

## Key Takeaway

**A thread consumes CPU only while running, but its latency can grow
while it waits runnable in a queue.**

## What to Remember

- A thread is, at any instant, running, runnable, or sleeping — only
  running time counts as CPU consumption, but runnable time still costs
  real latency.
- The run queue holds threads that are ready but waiting for a CPU;
  load balancing tries to keep that queue's work spread evenly.
- Time slices bound how long a thread runs uninterrupted when others are
  waiting; wake-up is a sleeping thread becoming runnable again.
- CPU utilization is an aggregate measure that cannot show whether any
  specific thread waited in the run queue before running.
- Involuntary context switches are a portable, directly measurable
  signal of runnable pressure, distinct from utilization.
- Scheduling class is a boundary concept worth knowing exists (real-time
  vs. time-shared policies) without needing full mastery for most
  workloads.

## Further Reading

- Linux `perf sched` documentation: <https://man7.org/linux/man-pages/man1/perf.1.html>
- `pidstat`(1) manual page.

## The Next Obvious Question

How do migrations and neighboring workloads add noise and delay?
