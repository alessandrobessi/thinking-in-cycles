# Bibliography

Seed list reproduced from BLUEPRINT.md Section 19 ("Initial technical
source backbone"), organized by category, plus a small set of additions
needed for Chapters 1-5's more conceptual material (the seed list is
perf/BPF/NUMA-heavy and doesn't cover benchmarking methodology or
queueing intuition on its own). Per Section 19's rules: technical claims
must be anchored in primary sources, and every hardware-specific event
description must be checked against the relevant architecture
documentation as those chapters are drafted.

## `perf` and profiling tools

- Linux perf documentation: <https://docs.kernel.org/admin-guide/perf/index.html>
- Linux workload tracing guide: <https://docs.kernel.org/admin-guide/workload-tracing.html>
- perf tutorial: <https://perfwiki.github.io/main/tutorial/>
- perf manual pages: <https://man7.org/linux/man-pages/man1/perf.1.html>
- FlameGraph project: <https://github.com/brendangregg/FlameGraph>

## BPF and dynamic tracing

- Linux BPF documentation: <https://docs.kernel.org/bpf/>
- libbpf overview: <https://docs.kernel.org/bpf/libbpf/libbpf_overview.html>
- bpftrace documentation: <https://bpftrace.org/docs/release_026/docs>
- BCC project and tools: <https://github.com/iovisor/bcc>

## Memory, coherence, and NUMA

- Linux false-sharing documentation: <https://docs.kernel.org/kernel-hacking/false-sharing.html>
- Linux NUMA overview: <https://docs.kernel.org/mm/numa.html>
- Linux NUMA memory policy: <https://docs.kernel.org/admin-guide/mm/numa_memory_policy.html>
- `numactl` manual: <https://man7.org/linux/man-pages/man8/numactl.8.html>

## Scheduling and affinity

- CPU affinity API: <https://man7.org/linux/man-pages/man2/sched_setaffinity.2.html>

## Vendor and benchmarking tools

- Intel Performance Counter Monitor: <https://github.com/intel/pcm>
- Google Benchmark user guide: <https://google.github.io/benchmark/user_guide.html>

## Additions for Chapters 1-5

The Section 19 seed list has no entry for general performance-workload
methodology or queueing intuition, both of which Part I depends on
directly. These two additions are flagged here for review rather than
silently folded into the seed list above.

- Brendan Gregg, *Systems Performance: Enterprise and the Cloud*, 2nd ed.
  (Addison-Wesley, 2020). Source for the USE Method (Chapters 1, 3) and
  general workload-characterization framing (Chapter 2). The single most
  directly relevant secondary source for this book's overall approach.
- Neil J. Gunther's writing on queueing intuition and Little's Law
  (see <https://en.wikipedia.org/wiki/Little%27s_law> for the formal
  statement; Gunther's *Guerrilla Capacity Planning* develops the
  practical intuition used informally in Chapter 3). No single
  canonical URL is given here deliberately — this is a placeholder for
  the editorial pass to pick a specific primary source before publication.
- Georges, Buytaert, and Eeckhout, "Statistically Rigorous Java
  Performance Evaluation," *OOPSLA* 2007. Canonical paper on
  benchmarking hygiene and the dangers of naive single-run comparisons,
  cited in Chapter 4 despite being about a different language runtime —
  the statistical argument is language-independent.

## Per-chapter reference stubs

`references/chapters/ch01-references.md` through `ch05-references.md`
point back into this file for the specific entries each drafted chapter
actually cites in its Further Reading section. Chapters 6-30 will get
their own stubs, and likely their own bibliography entries, as they are
drafted — see BLUEPRINT.md Section 19's rule that "the final bibliography
should add architecture-specific and chapter-specific sources."
