# Part I — Measuring Reality

Establishes the vocabulary and discipline every later part assumes:
workloads, resources, and the on-CPU/off-CPU accounting question
(Chapter 1); what "faster" actually means for a given workload (Chapter
2); the core measurement vocabulary of latency, throughput, utilization,
and saturation (Chapter 3); honest benchmarking under noise (Chapter 4);
and the investigation loop reused throughout the rest of the book
(Chapter 5).

| Chapter | Title | Opening Question |
|---|---|---|
| 1 | [Why Fast-Looking Code Runs Slowly](chapter-01-why-fast-looking-code-runs-slowly.md) | Why can a program feel slow when no obvious resource is fully used? |
| 2 | [Performance Is a Question, Not a Number](chapter-02-performance-is-a-question-not-a-number.md) | What exactly does "faster" mean for this workload? |
| 3 | [Latency, Throughput, Utilization, and Saturation](chapter-03-latency-throughput-utilization-saturation.md) | How do latency, throughput, utilization, and saturation differ? |
| 4 | [Noise, Variance, and Honest Benchmarks](chapter-04-noise-variance-and-honest-benchmarks.md) | How do we know a measured difference is real? |
| 5 | [The Performance Investigation Loop](chapter-05-the-performance-investigation-loop.md) | What investigation process prevents random tuning? |

All five guided labs are **portable**: no
root, no `perf`, no special hardware. They use `labs/cyclelab`'s
`compute` mode and the helper scripts in `labs/scripts/`.

Next: [Part II — What the CPU Is Doing](../part-2-what-the-cpu-is-doing/README.md) (Chapters 6-10).
