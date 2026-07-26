# Appendix E — Containers and cgroups

**Status:** reference material, not a chapter. Extends Chapter 23's
`cgroup`/`cpuset` material and Chapters 26-28's privilege requirements
into the specific complications a containerized environment adds on
top of both.

## Namespace versus host visibility

A container's process, network, and mount namespaces change *what a
process can see*, not what the underlying hardware is actually doing —
a critical distinction for performance work. A profiler running inside
a container's namespace typically sees only that container's own
processes and PIDs, which is usually the right scope for "profile my
application," but it also means host-level context (what else is
running on the same physical CPUs, whether a neighboring container is
the actual source of interference — Chapter 22's noisy-neighbor
territory) is invisible from inside the container's own namespace. Real
container-aware investigations frequently need a tool running with
host visibility (a privileged sidecar, or a host-level agent) precisely
to see the cross-container interference a purely in-container view
cannot.

## cgroup-scoped `perf`

Linux `perf` supports scoping a session to a specific cgroup directly
(`perf stat -G <cgroup> ...`), which is the practical way to answer
"what is this container's slice of the machine actually doing" without
needing to enumerate its PIDs manually or gain visibility into every
other container sharing the host. This is a direct extension of
Chapter 23's `cgroup` material — the same mechanism that constrains
*how much* CPU and memory a container can use also provides the
natural scope boundary for measuring what it actually used.

## cpusets and quotas

Chapter 23 already distinguishes CPU affinity (which logical CPUs a
thread *can* run on) from cgroup CPU constraints (how much CPU time a
group of processes is *allowed to consume*, via CFS quota/period or
cpuset pinning) — containers are where this distinction becomes
unavoidable in practice, since most container runtimes express
resource limits as cgroup settings by default. A container given a CPU
*quota* well below its host's full core count can show exactly Chapter
21's runnable-but-waiting signature (high involuntary context switches,
throughput well below what the raw hardware could deliver) even on an
otherwise idle host, because the quota itself — not contention with
other work — is throttling it. A container pinned to a *cpuset* instead
behaves more like Chapter 23's hard-affinity case: bounded to specific
CPUs, but not necessarily bounded in how much of their time it can use.
Checking which mechanism (or both) is in effect is the first step
before interpreting any container's own performance data.

## eBPF from host versus container

Chapters 26-28's tracing and eBPF material generally requires
capabilities (`CAP_SYS_ADMIN`, `CAP_BPF`, `CAP_PERFMON` depending on
kernel version) that are frequently *not* granted to an unprivileged
container by default, for good reason — the same broad, deep visibility
that makes eBPF powerful for observability also makes it a meaningful
container-escape and cross-tenant-visibility risk if handed out freely.
Practically, this means most real eBPF-based tracing of containerized
workloads runs from the *host*, with visibility into container
processes via the host's own view of the process tree, rather than
from inside each container's own restricted namespace. Where
in-container tracing is genuinely required, it needs deliberately
elevated capabilities granted to that specific container — a decision
Appendix G's production-safety discipline applies to directly (scope
the elevated access as narrowly as the actual question requires).

## Symbol paths

A profiler running on the host, symbolizing a process that lives inside
a container's mount namespace, needs to resolve that process's binaries
and libraries through the *container's* filesystem view, not the
host's — Chapter 13's symbol-resolution model assumes a single,
consistent filesystem; a container breaks that assumption unless the
profiling tool is specifically container-aware (resolving symbols via
`/proc/<pid>/root/...` or an equivalent container-filesystem-aware
path) or symbol resolution is done from inside the container's own
namespace instead.

## Kubernetes limitations

Kubernetes adds a further layer on top of plain containers: pod-level
resource requests/limits translate into the cgroup quotas described
above, but are usually configured through Kubernetes' own abstractions
rather than cgroup settings directly, so a performance investigation
often needs to trace from "pod is throttled" back to "which cgroup CPU
quota setting Kubernetes actually applied" before Chapter 21-23's
scheduling and affinity model becomes actionable. Running privileged,
host-level tooling (most of Chapters 26-28's real capability) inside a
Kubernetes cluster typically requires a `DaemonSet` with elevated
`securityContext` privileges explicitly granted — itself a
production-safety decision (Appendix G) that should be scoped as
narrowly as the actual investigation needs, not granted broadly "to be
safe."

## Related

- Chapter 21 (run queues, runnable-but-waiting — the signature a CPU
  quota throttle can produce), Chapter 22 (noisy neighbors, the
  cross-container version of interference), Chapter 23 (CPU affinity
  vs. cgroup constraints as separate mechanisms), Chapters 26-28
  (privilege requirements for tracing and eBPF).
- Appendix G (production safety — scoping privileged access narrowly,
  directly applicable to granting container/cluster-level tracing
  capabilities).
