# CPU Affinity, SMT, cpusets, and Isolation

**Part:** Part V — Where the Work Runs
**Concept level:** 5 (Topology and Placement — begins now, building on the scheduling mechanics of Chapters 21-22)
**Prerequisites:** run queue, migration, CPU pressure (Chapters 21-22)
**New concepts:** logical CPU, physical core, SMT sibling, socket, affinity mask, `taskset`, `sched_setaffinity`, cpuset, cgroup CPU constraints, isolation

## Opening Question

When does CPU affinity help, hurt, or merely hide a problem?

## Incident or Real-World Story

A team, frustrated by run-to-run benchmark variance (exactly Chapter
22's problem), reaches for a fix they've heard about: pin the benchmark
process to specific CPUs so the scheduler can't move it around. Variance
drops immediately, and the team adopts pinning as a standard practice
for every benchmark going forward, on every machine, without exception.
Months later, a different workload — one with more threads than the
pinned CPU set has room for — gets the same treatment by default, and
performance gets *worse*: threads that used to spread across the whole
machine now compete for a handful of pinned CPUs while the rest of the
machine sits idle. The fix that helped the first workload actively hurt
the second, because the two workloads needed opposite things from the
scheduler: the first wanted to stop it from moving a small number of
threads around; the second needed its freedom to spread many threads
across many CPUs.

Affinity isn't a universal improvement. It's a trade: give up the
scheduler's freedom to move things around, in exchange for control over
exactly where they run. Whether that trade is worth making depends
entirely on whether the chosen placement actually matches what the
workload needs — which the team's first success never actually
established, they just got lucky that pinning happened to suit that
specific workload's shape.

## Predict Before Measuring

Before reading further: if you pin twice as many runnable threads as
there are CPUs in the pinned set, while leaving other CPUs on the
machine completely idle, what do you expect to happen to those threads'
performance compared to leaving them unpinned and free to use the whole
machine?

## Core Intuition

A **logical CPU** is one schedulable unit from the OS's perspective — on
a machine with simultaneous multithreading, one **physical core** can
present as more than one logical CPU (**SMT siblings**, sharing the
core's execution resources despite appearing as separate CPUs to the
scheduler). A **socket** is a physical CPU package, potentially
containing many cores. An **affinity mask** specifies exactly which
logical CPUs a thread is allowed to run on — the canonical picture is
assigning a worker to a specific workstation instead of letting them
use whichever bench is free; it guarantees where they'll be, at the
cost of the flexibility to shift them elsewhere when their usual bench
is busy. `taskset` (a command-line tool) and `sched_setaffinity` (the
underlying system call) are how Linux sets one. A **cpuset** groups
CPUs (and memory nodes) into a named
partition threads can be restricted to, often managed today through
**cgroup CPU constraints** — the mechanism container CPU limits are
usually built on. **Isolation** (`isolcpus` and related boot-time or
runtime configuration) goes further, trying to keep specific CPUs mostly
free of the kernel's own routine work, reserved for latency-sensitive
threads.

## Technical Explanation

The mandatory cautions this chapter insists on, all illustrated by this
chapter's opening story or direct extensions of it: **affinity can block
load balancing** — a pinned thread can't be moved even when doing so
would relieve real contention elsewhere, exactly what hurt the second
workload in the story. **CPU affinity does not automatically bind
memory** — a thread pinned to a CPU on one socket can still have its
memory allocated on a different socket's memory controller, a mismatch
Chapter 24 covers directly. **Container CPU limits and cpusets can
override what you think you've configured** — a process's own affinity
request can be silently constrained by an outer cgroup limit it has no
visibility into. And **`isolcpus` and low-latency boot tuning are
advanced operational choices**, not a first-line fix for ordinary
variance — they change how the *entire machine* schedules routine
kernel work, a much bigger commitment than pinning one benchmark.

## Tool View

- What is measured: this chapter's portable lab checks what affinity
  control is actually available on the machine running it, and measures
  natural run-to-run variance in its absence.
- What is not measured: the actual effect of hard pinning — genuinely
  untestable on this book's macOS reference machine, which has no
  user-accessible hard-affinity API (confirmed directly by `cyclelab`'s
  own `--affinity` flag, built exactly to report this honestly rather
  than silently pretend to pin — see `labs/cyclelab/README.md`).
- Required permissions: none for this chapter's portable lab; on Linux,
  `taskset`/`sched_setaffinity` typically work unprivileged for a
  process's own threads, though cgroup/cpuset configuration often needs
  elevated privileges.
- Likely overhead: negligible for checking or setting affinity itself.
- Portability: on Linux, the guided experiments this chapter is built
  around are directly runnable:

  ```bash
  # Pin one thread and compare variance against unpinned:
  taskset -c 0 ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=1

  # Place two CPU-heavy threads on SMT siblings vs. separate cores
  # (exact sibling pairs are CPU-specific -- check `lscpu -e`):
  taskset -c 0,1 ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=2   # e.g. siblings
  taskset -c 0,4 ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=2   # e.g. separate cores

  # Demonstrate oversubscription within a pinned set (Chapter 21's
  # lesson, made deliberate instead of accidental):
  taskset -c 0,1 ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=8
  ```

  **Documented, not tested** on this book's macOS reference machine.
- Common failure mode: pinning as a reflexive fix for variance (this
  chapter's opening story) without checking whether the pinned set
  actually has room for the workload's thread count.

## Guided Lab

**Portability:** portable (this chapter's actual lab, checking
availability and measuring unpinned variance); the Linux experiments
above are **hardware-dependent** / **privileged** in the cpuset case.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch23_affinity_availability.sh
```

**Expected qualitative result:** on a machine without hard-affinity
support, `cyclelab` should report the limitation and continue rather
than fail; repeated runs should show some natural variance from the
scheduler's own, uncontrollable placement decisions. One example run on
the reference machine for this book (Apple M4, macOS, arm64) showed:

```text
cyclelab: warning: CPU affinity pinning is not supported on this OS; running without pinning
performance cores: 4    efficiency cores: 6
thread_count=10 core_count=10 -- equal means no SMT on this chip

rep    throughput_ops_s
1      481,640,221
2      469,925,190
3      458,372,900
4      456,584,412
5      518,661,684
6      504,144,499
```

Roughly 12% spread between the lowest and highest repetition, with no
way on this machine to test whether pinning would have reduced it.

**Interpretation:** this machine's heterogeneous performance/efficiency
core split is itself worth noting even without hard pinning available:
on such a design, *which* cores a thread lands on can matter as much as
*how many* — a consideration Linux's `taskset` would let you control
directly and this chapter's portable lab can only observe indirectly,
through variance.

**Fallback path:** this chapter's entire lab already is the fallback
path for machines without hard-affinity support; on Linux, run the Tool
View section's `taskset` experiments directly instead.

**Cleanup:** none.

## Common Misconceptions

### *"Pinning always improves performance." (M09)*

**Why it's wrong:** Affinity trades scheduler freedom for placement
control, and removing that freedom can increase queueing when a pinned
thread count exceeds the pinned CPU set's capacity — this chapter's
opening story directly.

**Correct intuition:** Compare a workload's performance pinned versus
unpinned *at its actual thread count*, not assumed from a different
workload's earlier success with pinning.

**Analogy:** Assigning every employee a permanently fixed desk sounds
like it should make an office more efficient, but if you assign twelve
people to a room with eight desks, you've just created a line for
chairs that free seating never would have had.

### *"CPU affinity also binds memory." (M10)*

**Why it's wrong:** CPU and memory placement are separate policies on
NUMA systems — pinning a thread to a CPU says nothing about where its
memory physically lives.

**Correct intuition:** Chapter 24 and 25's NUMA placement tools operate
independently of CPU affinity tools, precisely because the two are
orthogonal.

**Analogy:** Assigning a worker to a specific workstation doesn't
automatically move their filing cabinet there too — someone still has
to decide, separately, where the actual files live.

## Practical Implications

Before pinning a workload as a fix for variance or as a default
practice, check whether the pinned CPU set actually has room for the
workload's thread count, and whether the workload's memory placement
(Chapter 24) needs to be controlled alongside its CPU placement to
actually deliver the intended benefit — pinning CPUs alone, without
memory, can leave the original problem partly in place.

## Key Takeaway

**Affinity trades scheduler freedom for placement control; it helps
only when the chosen topology matches the workload.**

## What to Remember

- A logical CPU is what the scheduler sees; SMT siblings let one
  physical core present as more than one logical CPU.
- Affinity masks (`taskset`/`sched_setaffinity`) restrict which logical
  CPUs a thread may run on; cpusets and cgroup CPU constraints do the
  same at a group level, often beneath container CPU limits.
- Pinning removes the scheduler's ability to move a thread away from
  contention — a benefit only if the pinned set has enough room for the
  workload.
- CPU affinity and memory placement are separate policies; pinning one
  does not pin the other (M10).
- Container CPU limits and cpusets can silently override an
  application's own affinity requests.
- `isolcpus` and boot-time isolation are machine-wide, advanced choices,
  not a substitute for right-sizing a workload's own thread count.

## Further Reading

- `sched_setaffinity`(2) manual page: <https://man7.org/linux/man-pages/man2/sched_setaffinity.2.html>
- `taskset`(1) manual page.

## The Next Obvious Question

Why is some memory farther away than other memory?
