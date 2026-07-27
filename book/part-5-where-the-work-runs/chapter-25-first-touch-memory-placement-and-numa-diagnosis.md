# First-Touch, Memory Placement, and NUMA Diagnosis

**Part:** Part V — Where the Work Runs
**Concept level:** 5
**Prerequisites:** NUMA node, local memory, remote memory, distance (Chapter 24)
**New concepts:** first-touch allocation, local allocation policy, interleave, bind, preferred node, automatic NUMA balancing, page migration, NUMA hit/miss statistics

## Opening Question

How do allocation and execution placement interact on NUMA systems?

## Incident or Real-World Story

A service initializes a large in-memory data structure once, at startup,
on a single setup thread — a common, reasonable pattern — and then
hands it off to a pool of worker threads that process it for the
remainder of the program's life. On a NUMA machine, this pattern has a
specific, non-obvious failure mode: whichever socket the setup thread
happened to run on is where every page of that structure gets
physically placed, because memory typically isn't actually backed by
physical pages until the moment something first writes to it — and
that's the setup thread, once, at startup. If the worker pool later
runs across *all* sockets (entirely reasonable — Chapter 21's load
balancing doing exactly what it's supposed to), every worker not on the
setup thread's original socket spends the rest of the program's life
reading that entire structure as remote memory.

Nothing about this shows up as a bug. The data is correct. The failure
mode is purely a placement mismatch created at the exact moment of first
write, invisible unless you know to ask "which socket touched this
memory first, and which sockets are using it now."

## Predict Before Measuring

Before reading further: on a two-socket NUMA machine, if a single setup
thread on socket 0 initializes a large buffer, and then four worker
threads — two pinned to socket 0, two pinned to socket 1 — each process
an equal share of that same buffer, do you expect all four workers to
see the same average memory latency, or do you expect a split? Which two
workers do you expect to be faster?

## Core Intuition

**First-touch allocation** is the common default policy: a page of
memory isn't actually assigned to a specific physical location until
the first time something writes to it, and at that moment, it's placed
on the node local to whichever CPU did the writing — this chapter's
opening story is first-touch operating exactly as designed, just not as
the programmer expected. A **local allocation policy** requests
first-touch behavior explicitly; **interleave** instead spreads a
buffer's pages round-robin across multiple nodes deliberately, trading
worst-case remote access for a more even, predictable aggregate load;
**bind** forces allocation onto one specific node regardless of which
CPU touches it first; a **preferred node** is a softer hint — try this
node, fall back if it's full. **Automatic NUMA balancing** is the
kernel's own background effort to notice a page being accessed
predominantly from a remote node and consider **page migration** —
physically moving it closer — weighing that against the real cost of
the move itself. **NUMA hit/miss statistics** report *page-allocation*
outcomes — whether a page was actually allocated on the node its policy
preferred (`numa_hit`) or had to fall back to a different node
(`numa_miss`), accumulated since the process started — not a live tally
of individual memory accesses. They are the right evidence for "did
placement happen the way this policy intended," a necessary precondition
for good locality; they are not themselves a measurement of how many
memory *accesses* ended up local versus remote, which no counter at this
layer reports directly — that evidence has to come from timing (Chapter
24's local-versus-remote latency gap) or hardware counters (Chapter 20),
not from allocation statistics.

## Technical Explanation

The mechanism this chapter's story turns on is precisely the interaction
between *when* a page is first touched and *where* the threads using it
later actually run. First-touch is a sensible default because it's
often right by accident — a thread that allocates its own working data
and then immediately uses it will naturally touch and use memory from
the same node. It becomes wrong specifically when allocation and use are
separated across threads or across time, as in this chapter's story,
because the policy has no way to know that the *later* access pattern
will differ from the *first* one. Interleaving trades away the
best-case (everything local) for a more predictable worst-case (every
thread pays roughly the same partial remote-access cost, rather than
some threads paying nothing and others paying the full cost) — useful
specifically when access is genuinely spread across many threads on many
nodes and no single "right" node exists. Automatic NUMA balancing tries
to fix mismatches after the fact, but page migration itself isn't free —
it can help a stable, long-running mismatch and actively hurt a workload
whose access pattern changes faster than migration can track it.

## Tool View

- What is measured: this chapter has no portable, tested lab component —
  every technique here (first-touch control, interleaving, binding,
  automatic balancing, hit/miss statistics) is either a Linux kernel
  policy or a Linux-specific tool, meaningless on a single-node machine
  where every access is already local by construction.
- What is not measured: nothing new is captured on this book's
  reference machine beyond Chapter 24's own confirmation that it has no
  NUMA topology to place memory across in the first place.
- Required permissions: `numactl` itself needs no special privileges for
  a process's own memory policy; reading other processes' NUMA
  statistics may need elevated access depending on system configuration.
- Likely overhead: negligible for setting policy; automatic page
  migration has a real, if usually modest, cost each time it moves a
  page.
- Portability: Linux-only, and only meaningful on genuinely multi-node
  hardware:

  ```bash
  numactl --cpunodebind=0 --membind=0 ./labs/cyclelab/bin/cyclelab random-memory --working-set-size=512M --duration=2 --threads=4
  numactl --cpunodebind=1 --membind=0 ./labs/cyclelab/bin/cyclelab random-memory --working-set-size=512M --duration=2 --threads=4
  numactl --interleave=all ./labs/cyclelab/bin/cyclelab bandwidth --duration=2 --threads=8
  numastat -p "$PID"   # substitute the target process's PID
  ```

  The first two commands are designed to be compared directly: same
  workload, same memory-binding node, only the *executing* node differs —
  isolating exactly the local-versus-remote-execution question this
  chapter's Predict Before Measuring section asked. `random-memory`'s
  dependent pointer chase over a working set well beyond any cache level
  (Chapter 16) is the right workload for this specific comparison,
  deliberately not `compute`: a tiny, register- and L1-resident
  accumulator would settle into cache after its first few iterations and
  stop touching main memory at all, at which point CPU/memory-node
  binding would have nothing left to measure. **All four commands are
  documented, not tested**, consistent with Chapter 24's confirmed
  single-node reference machine.

  One instrumentation caveat: `numastat -p <pid>` reports page-placement
  and allocation counters (`numa_hit`, `numa_miss`, and related fields) —
  whether the pages a process is using were actually allocated on that
  process's preferred or requested node, accumulated since the process
  started. It does not report a live tally of which fraction of memory
  *accesses* were satisfied locally versus remotely; no per-access
  counter like that exists at this layer. In this comparison, what
  actually demonstrates the local-versus-remote cost difference is the
  elapsed-time (or `ns_per_access`) gap between the two `--cpunodebind`
  runs themselves; `numastat -p` is useful alongside that for confirming
  the pages genuinely landed where `--membind` asked them to (a
  placement check), not as a substitute measurement of access locality
  itself.
- Common failure mode: pinning CPU placement (Chapter 23) without also
  addressing memory placement, leaving the exact mismatch this chapter's
  story describes fully intact — CPU affinity alone never touches memory
  policy (M10, Chapter 23).

## Guided Lab

**Portability:** **bare-metal recommended** / hardware-dependent — this
chapter's mechanism requires genuine multi-node NUMA hardware to observe
directly, which this book's reference machine does not have (Chapter
24). In place of a measured lab, work through this reasoning exercise,
in the spirit of Section 24's "Predict Before Measuring" and the
narrower question above:

1. State a specific hypothesis for the Predict Before Measuring
   scenario: which two of the four workers (socket-0-pinned or
   socket-1-pinned) do you expect to show lower average memory latency,
   and by roughly how much relative to the other two, given everything
   Chapter 24 established about local versus remote access cost?
2. Write down what evidence would confirm or refute that hypothesis:
   primarily the elapsed-time (or `ns_per_access`) gap between the two
   `--cpunodebind` runs, since that gap *is* the local-versus-remote
   cost difference. Separately, write down what you'd want `numastat -p
   <pid>`'s `numa_hit`/`numa_miss` fields to show as a placement check —
   confirmation that each run's pages actually landed on the node
   `--membind` requested — keeping in mind that those fields count page
   allocations, not individual memory accesses, so they corroborate
   *where the memory is*, not *how fast reaching it was*.
3. State what changing the setup thread's pinning (rather than the
   workers') would be expected to do to the same experiment, and why
   that's a different fix than pinning the workers themselves.

**Expected qualitative result:** a testable, falsifiable prediction for
each of the three steps above — the same discipline Chapter 5
established, applied here to a mechanism this book cannot run for you
directly on its own reference hardware.

**Interpretation:** the reasoning itself is the deliverable this
chapter's fallback path returns — if you have access to genuine
multi-socket Linux hardware, the Tool View section's four commands are
exactly what to run to check your answers against real measurements.

**Fallback path:** this reasoning exercise *is* the fallback path,
consistent with Chapter 24's confirmed single-node status; there is no
further portable substitute for genuinely multi-node hardware here.

**Cleanup:** none.

## Common Misconceptions

### *"CPU affinity also binds memory." (M10, revisited)*

**Why it's wrong:** This chapter's own opening story is the sharpest
possible version of this misconception's consequence: pinning worker
threads to specific sockets (Chapter 23) does nothing at all to fix a
first-touch mismatch, because CPU affinity and memory placement are
separate policies enforced by separate mechanisms —
`taskset`/`sched_setaffinity` control the former, `numactl`/memory-policy
calls control the latter.

**Correct intuition:** A team that pins CPUs and stops there, believing
the placement problem is solved, has fixed half of a two-part problem
and left the other half — which memory node actually holds the data —
completely untouched.

**Analogy:** Reassigning a worker to a desk closer to the supply closet
doesn't move the supplies they already have stacked at their old desk —
someone still has to carry the boxes over separately.

### *"NUMA matters only at enormous scale." (M11, revisited)*

**Why it's wrong:** The setup-thread/worker-pool pattern this chapter
opens with needs only two sockets to produce a real, measurable
remote-memory penalty for half a worker pool — it is not a problem that
only appears on machines with many nodes.

**Correct intuition:** Any multi-node system, however small, has
exactly the same local/remote distinction Chapter 24 introduced.

**Analogy:** Setting up a shared kitchen on the wrong floor of a
two-story duplex is just as inconvenient, proportionally, as putting it
on the wrong floor of a forty-story building — you don't need forty
floors for "which floor" to matter.

## Practical Implications

Before assuming a NUMA-aware placement problem is fixed by pinning CPUs
alone, check where the relevant memory was actually first touched, and
by which thread — a setup/worker split, exactly this chapter's story, is
a common, easy-to-miss source of a placement mismatch that CPU affinity
alone cannot fix. When access truly is spread evenly across many
threads on many nodes, interleaving is worth considering over strict
local binding, trading best-case latency for more predictable aggregate
behavior.

## Key Takeaway

**NUMA performance follows the relationship between where pages are
first placed and where the threads that use them actually run.**

## What to Remember

- First-touch allocation places a page on the node local to whichever
  thread first writes to it, not necessarily the node that will use it
  most.
- A setup-thread/worker-pool split is a common, easy-to-miss way
  first-touch placement stops matching later access patterns.
- Interleaving spreads pages across nodes deliberately, trading
  best-case local latency for more predictable aggregate cost.
- Binding forces allocation onto a specific node; a preferred node is a
  softer version of the same request.
- Automatic NUMA balancing can migrate pages to fix a stable mismatch,
  but migration itself has a real cost, and can hurt workloads whose
  access patterns shift faster than it can track.
- CPU affinity and memory placement are separate levers (Chapter 23's
  M10) — fixing one without the other can leave the original mismatch
  fully intact.

## Further Reading

- Linux NUMA memory policy: <https://docs.kernel.org/admin-guide/mm/numa_memory_policy.html>
- `numactl`(8) manual page: <https://man7.org/linux/man-pages/man8/numactl.8.html>

## The Next Obvious Question

What can dynamic tracing observe that counters and sampling cannot?
