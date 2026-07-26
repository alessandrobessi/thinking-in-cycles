# NUMA: Distance Inside One Machine

**Part:** Part V — Where the Work Runs
**Concept level:** 5
**Prerequisites:** logical CPU, physical core, socket (Chapter 23)
**New concepts:** NUMA node, local memory, remote memory, socket, interconnect, distance, node topology, memory-only node

## Opening Question

Why is some memory farther away than other memory?

## Incident or Real-World Story

A service running on a large, multi-socket machine gets measurably
faster the moment someone adds one line to its startup configuration —
no code change, no algorithm change, just a placement hint telling the
OS to keep the service's threads and its memory on the same socket.
Before that change, the service's threads were being scheduled freely
across both sockets (exactly the load-balancing freedom Chapter 21
praised), while its memory had all been allocated on whichever socket
happened to be handling requests first. Half the time, a thread on
socket 1 was reading memory that physically lived next to socket 0 —
correct, consistent, and measurably slower than reading memory next to
whichever socket was actually running the thread.

Nothing about this was a bug. Every access still returned the right
data. The cost was invisible in every functional test and only showed
up as elevated latency once someone specifically asked whether *where*
the memory lived mattered as much as *how much* of it there was —
exactly the question this chapter exists to raise.

## Predict Before Measuring

Before reading further: on a machine with two CPU sockets, each with
its own directly attached memory, would you expect a CPU on socket 0
reading memory attached to socket 1 to be exactly as fast as reading
memory attached to its own socket, meaningfully slower, or does it
depend entirely on the workload? Hold your prediction — the mechanism
this chapter describes has a definite answer.

## Core Intuition

A **NUMA node** (Non-Uniform Memory Access) is a group of CPUs and the
memory directly attached to them — commonly, though not always, one
node per **socket**. The canonical picture: multiple workshops, each
with its own local storeroom, connected to each other by a corridor —
fetching from your own workshop's storeroom is quick; fetching from
another workshop's storeroom means a walk down the corridor first. CPU
placement (which workshop a task happens in) and memory placement
(which storeroom holds the material) stay conceptually distinct even in
the analogy, exactly as they must in practice (Chapter 23's M10).
**Local memory** is memory attached to the same
node as the CPU accessing it; **remote memory** is memory attached to a
*different* node, reachable only by crossing an **interconnect** — a
physical link between sockets that is slower and lower-bandwidth than a
socket's own direct memory connection. **Distance** is a relative,
often unitless number the system reports for how costly reaching a
given node's memory is from a given CPU — larger means farther, both
literally and in latency. **Node topology** is the overall map of how
many nodes exist and how they're connected; a **memory-only node** (no
CPUs attached, memory reachable only remotely) is a real configuration
on some systems, worth knowing exists even without deep treatment here.

## Technical Explanation

The core fact this chapter's opening story turns on: on a NUMA machine,
the *same instruction* — a memory read — costs a different amount
depending purely on which node's memory it's reaching, holding
everything else constant. This is the multi-socket, hardware-topology
version of Chapter 16's cache hierarchy: just as an L1 hit costs less
than a DRAM access, a local DRAM access costs less than a remote one —
another rung on the same "how far away is this data" ladder, at a
larger physical scale. Execution placement (which CPU runs a thread,
Chapter 23's subject) and memory placement (which node a thread's data
lives on) are two separate decisions that jointly determine whether a
given access is local or remote — matching each other well is the whole
game, and Chapter 25 covers how.

## Tool View

- What is measured: this chapter's portable lab checks this machine's
  own NUMA topology (or lack of it) directly, using
  `scripts/doctor.sh`'s own NUMA-detection section.
- What is not measured: actual local-vs-remote latency — genuinely
  untestable without multi-socket NUMA hardware, which this book's
  macOS reference machine does not have (a single-package Apple Silicon
  design with no discrete NUMA nodes in the traditional sense).
- Required permissions: none.
- Likely overhead: none; topology inspection is a read-only query.
- Portability: on Linux, with genuinely multi-node hardware:

  ```bash
  lscpu                                          # summary, includes NUMA node count
  lscpu -e                                       # per-CPU detail, including which node each belongs to
  numactl --hardware                             # node count, memory per node, and the distance matrix
  cat /sys/devices/system/node/node*/distance    # raw distance values, node by node
  ```

  **Documented, not tested** on this book's macOS reference machine. A
  schematic (illustrative, not captured) two-socket `numactl --hardware`
  distance matrix looks like:

  ```text
  node distances:
  node   0   1
    0:  10  21
    1:  21  10
  ```

  Reading local (self) distance as 10 and cross-socket distance as 21 is
  a common convention (not a universal constant) — the exact numbers
  vary by system, but a distinctly higher remote value than the local
  one is the pattern to expect.
- Common failure mode: assuming every machine has NUMA topology worth
  investigating, or the opposite — assuming NUMA is exclusively a
  many-socket-server concern irrelevant to smaller or single-package
  systems (M11, below).

## Guided Lab

**Portability:** portable (this chapter's actual lab, topology
detection); the Linux commands above are **bare-metal recommended** /
hardware-dependent for a genuinely multi-node result.

**Setup:** none beyond having this repository's `scripts/doctor.sh`.

**Command:**

```bash
./scripts/doctor.sh
```

(Already introduced in Section 13.3 / Chapter 1's setup — this chapter
is the first to specifically care about its NUMA section.)

**Expected qualitative result:** on a single-node machine, `doctor.sh`
should report NUMA as not applicable, cleanly, rather than crashing or
fabricating fake node data. On a genuinely multi-node Linux machine, it
should report `numactl` availability and the node count found. One
real run on the reference machine for this book (Apple M4, macOS,
arm64) showed:

```text
== NUMA ==
  [SKIP] not applicable on Darwin (no NUMA concept outside Linux here)
```

**Interpretation:** this is a real, honest answer, not a limitation to
work around — this specific machine has no NUMA topology to inspect,
and this book's own policy for exactly this situation is explicit:
on a single-node machine, treat multi-socket example data (like the
schematic distance matrix in Tool View) as a teaching aid, not as
performance evidence collected from this machine. If you have access to
genuinely multi-socket Linux hardware, rerun `doctor.sh` there and
follow up with the `lscpu`/`numactl` commands above for a real distance
matrix.

**Fallback path:** already this chapter's primary path — no
multi-socket hardware is required to complete it, only to extend it
with real captured data.

**Cleanup:** none.

## Common Misconceptions

**M11 — "NUMA matters only at enormous scale."** This is wrong because
any multi-node system — including modest dual-socket workstations and
many mainstream servers, not just large supercomputers — can suffer
remote-memory cost and bandwidth imbalance; node count, not raw core
count or price tier, is what determines whether NUMA effects apply. The
evidence that distinguishes the two: check node count directly
(`numactl --hardware` or this chapter's `doctor.sh` check) rather than
assuming from a machine's size or role — a compact dual-socket server
has exactly the same local/remote distinction as a much larger one.

## Practical Implications

Before assuming a performance investigation on unfamiliar hardware can
ignore memory topology, check node count directly rather than guessing
from the machine's category. On confirmed single-node hardware, this
entire chapter's mechanism doesn't apply and can be set aside
correctly, not just conveniently — the check itself is what makes that
a defensible decision rather than an assumption.

## Key Takeaway

**NUMA makes memory placement part of execution placement: the same
address can cost more depending on which CPU accesses it.**

## What to Remember

- A NUMA node groups CPUs with their directly attached, local memory;
  reaching another node's memory means crossing a slower interconnect.
- Distance is a relative measure of that cost, reported per node pair;
  higher means farther and generally more costly.
- Execution placement (which CPU) and memory placement (which node) are
  separate decisions that must match for an access to be local.
- A memory-only node (no CPUs, memory reachable only remotely) is a
  real, if less common, configuration worth knowing exists.
- NUMA effects apply to any multi-node system, not only
  supercomputer-scale hardware (M11).
- Checking node count directly is the right first step before assuming
  either that NUMA matters or that it doesn't.

## Further Reading

- Linux NUMA overview: <https://docs.kernel.org/mm/numa.html>
- `numactl`(8) manual page: <https://man7.org/linux/man-pages/man8/numactl.8.html>

## The Next Obvious Question

How do allocation and execution placement interact on NUMA systems?
