# Part VI — Seeing the Invisible

Completes this book's model of observation itself: what dynamic tracing
can see that counters and sampling cannot (Chapter 26), how eBPF makes
custom, safe, in-kernel measurement possible (Chapter 27), how
`bpftrace` and BCC turn a precisely stated question into live
instrumentation (Chapter 28), how a complete latency explanation must
account for time a thread spends off-CPU, not just on it (Chapter 29),
and how every layer this book built — workload design, counters,
profiles, memory analysis, topology, and tracing — combines into one
defensible investigation (Chapter 30).

| Chapter | Title | Opening Question |
|---|---|---|
| 26 | [Events, Tracepoints, Kprobes, and Uprobes](chapter-26-events-tracepoints-kprobes-and-uprobes.md) | What can dynamic tracing observe that counters and sampling cannot? |
| 27 | [The eBPF Mental Model](chapter-27-the-ebpf-mental-model.md) | How can eBPF safely run custom measurements inside the kernel? |
| 28 | [bpftrace and BCC: Questions as Programs](chapter-28-bpftrace-and-bcc-questions-as-programs.md) | How do `bpftrace` and BCC turn questions into live instrumentation? |
| 29 | [Off-CPU Time, I/O Latency, and Contention](chapter-29-off-cpu-time-io-latency-and-contention.md) | Where does time go when a thread is not on a CPU? |
| 30 | [The Complete Linux Performance Investigation](chapter-30-the-complete-linux-performance-investigation.md) | How do all the layers combine into one defensible investigation? |

This Part's guided-lab portability is the widest split in the book.
Chapter 26's own real, tested finding: neither `dtrace` (present but
requiring elevated privileges under this reference machine's System
Integrity Protection) nor `bpftrace`/BCC (absent entirely — Linux-only)
is usable unprivileged here, confirmed directly by
`scripts/doctor.sh`'s "dtrace" and "bpftrace / BCC" sections and by
`ch26_probe_availability.sh`. Chapters 26-28's Linux tracing/eBPF
commands are therefore documented against each tool's stable interface,
not tested against real captured output, following the same honest
pattern established for `perf` throughout Parts II-IV. Chapter 29
recovers real, dramatic, tested content anyway: two new `cyclelab`
modes built for this Part — `lock-contention` (a genuinely serializing,
blocking workload) and `sleep` (an intentionally off-CPU one) — combined
with macOS's `sample`(1), which (unlike Linux `perf record`'s on-CPU-only
default) captures every thread's stack on a wall-clock interval
regardless of run state. On this reference machine, a `lock-contention`
capture showed 77.8% of sampled stack frames genuinely blocked inside a
mutex wait, versus 0.0% for an equivalent `compute`-mode capture — real,
reproducible off-CPU evidence, without any Linux-only tracing
infrastructure. Chapter 30's case study reuses this real data (plus
fresh `bandwidth`-mode measurements) for six of its eight steps, with
the two NUMA-dependent steps (thread/memory placement) documented as a
schematic continuation, consistent with Chapters 24-25's own honesty
about this machine's single-node topology.

This also completes the book: all thirty chapters across all six Parts
are now drafted.
