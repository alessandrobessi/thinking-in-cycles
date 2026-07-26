# Part V — Where the Work Runs

Returns to Linux execution mechanics (first touched informally in
Chapter 1) now with the full CPU and memory model from Parts II-IV
available: how the scheduler decides where runnable work executes
(Chapter 21), the real cost of context switches, migrations, and
sharing a machine with unrelated work (Chapter 22), when pinning
threads to specific CPUs helps versus hurts (Chapter 23), why some
memory is physically farther away than other memory on multi-socket
hardware (Chapter 24), and how allocation and execution placement
interact on NUMA systems (Chapter 25).

| Chapter | Title | Opening Question |
|---|---|---|
| 21 | [The Scheduler, Run Queues, and CPU Time](chapter-21-the-scheduler-run-queues-and-cpu-time.md) | How does Linux decide where runnable work executes? |
| 22 | [Context Switches, Migrations, and Interference](chapter-22-context-switches-migrations-and-interference.md) | How do migrations and neighboring workloads add noise and delay? |
| 23 | [CPU Affinity, SMT, cpusets, and Isolation](chapter-23-cpu-affinity-smt-cpusets-and-isolation.md) | When does CPU affinity help, hurt, or merely hide a problem? |
| 24 | [NUMA: Distance Inside One Machine](chapter-24-numa-distance-inside-one-machine.md) | Why is some memory farther away than other memory? |
| 25 | [First-Touch, Memory Placement, and NUMA Diagnosis](chapter-25-first-touch-memory-placement-and-numa-diagnosis.md) | How do allocation and execution placement interact on NUMA systems? |

This Part's guided-lab portability is the most varied so far. Chapters
21-23 are **portable**, built on a new cross-cutting `cyclelab`
feature — every mode now reports process-wide voluntary/involuntary
context-switch counts via POSIX `getrusage(2)`, working identically on
Linux and macOS — which makes real, reproducible scheduling-pressure and
noisy-neighbor experiments possible without `perf sched` or `pidstat`.
Chapters 24-25 are **bare-metal recommended** / hardware-dependent:
this book's reference machine (Apple M4, macOS, arm64) has no NUMA
topology at all — confirmed directly by `scripts/doctor.sh`'s own NUMA
check — so both chapters follow this book's own explicit portability
allowance to use clearly-marked schematic multi-socket data as a
teaching aid, with Chapter 25's lab replaced by a structured
prediction/reasoning exercise in place of a measurement this hardware
cannot produce.

Next: [Part VI — Seeing the Invisible](../part-6-seeing-the-invisible/README.md) (Chapters 26-30).
