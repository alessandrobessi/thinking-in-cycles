# Why Fast-Looking Code Runs Slowly

**Part:** Part I — Measuring Reality
**Concept level:** 0 (informally previews Level 7 on-CPU/off-CPU vocabulary; see `concept-graph.md`)
**Prerequisites:** none — this is the opening chapter
**New concepts:** workload, resource, on-CPU, off-CPU, bottleneck, critical path

## Opening Question

Why can a program feel slow when no obvious resource is fully used?

## Incident or Real-World Story

A small internal service — call it the pricing API — sits behind a
dashboard that product managers check every morning. One week, the
dashboard starts taking four to six seconds to load instead of one. An
engineer opens a monitoring panel and finds nothing alarming: the pricing
API's CPU utilization hovers around 20%. Memory is flat. Disk isn't full.
The load balancer isn't dropping anything. Every graph an on-call engineer
knows how to read says the machine is fine.

The engineer's first instinct is to look for what changed in the code, and
finds nothing — no new deploys that week. The second instinct is to add
more machines, on the theory that more CPU capacity can't hurt. It doesn't
help: the new machines show the same 20% utilization and the same slow
responses.

What's actually happening only becomes visible when someone looks at a
single request end to end instead of the machine as a whole. Each request
to the pricing API touches a small in-memory cache guarded by a lock,
then makes a call to a downstream inventory service, then writes a log
line to a slow, contended disk. On the slow days, three worker threads
handling concurrent requests spend most of their time waiting: one is
blocked on the lock, one is blocked waiting for the inventory service to
respond, one is blocked on the disk write. None of that waiting shows up
as CPU usage. A thread that isn't running isn't consuming a visible
resource on a CPU utilization graph — it's just not making progress. The
machine looks idle because, from the CPU's point of view, it mostly is.

The bug, if there is one, isn't in any function. It's in the assumption
that a resource has to be visibly pinned at 100% before it can be the
reason a request is slow.

## Predict Before Measuring

Before reading further: if a single-threaded program spends half its
wall-clock time waiting on a network call and half its time actually
computing, what will `top` show for that process's CPU usage while it
runs? Will it show 100%, 50%, or something else? Write down an answer
before continuing — you'll check it in the Guided Lab.

## Worked Example

Consider two versions of the same small task, run on the same machine:

- **Version A** runs a tight numeric loop for exactly one second, keeping
  a single CPU fully busy the entire time.
- **Version B** does the same amount of useful arithmetic, but that
  arithmetic takes only 200 milliseconds; the rest of its one-second
  wall-clock time is spent waiting — for a lock, a socket read, a sleep,
  it doesn't matter which.

Both versions take the same one second to finish. Only one of them will
show up as "busy" on a CPU-utilization graph in a way that matches how
long it actually took. If you only ever measured "is the CPU busy," you
would conclude Version B's CPU is idle 80% of the time — true — but you
would learn nothing about why the *task* took a full second. The missing
80% went somewhere. This chapter's job is to insist that "somewhere" is
answerable, not mysterious.

## Core Intuition

A running program is a **workload**: a defined, reproducible unit of work,
not an abstract "the program." At every instant, each thread of that
workload is doing exactly one of two things: it is **on-CPU** (actually
executing on a processor) or it is **off-CPU** (waiting for something —
a lock, an I/O operation, a scheduling decision, a timer, anything).
Elapsed time is the sum of on-CPU time and off-CPU time. Nothing else
exists to account for.

A **resource** is anything a workload can compete for or wait on: a CPU, a
lock, a network connection, a disk, memory bandwidth. A **bottleneck** is
whichever resource or step currently limits how fast the workload as a
whole can go. The **critical path** is the specific chain of dependent
steps — one thread waiting on another, one call waiting on a reply — whose
total length sets the minimum possible time the request could take,
because none of those steps can overlap with each other.

The pricing API story is an accounting problem, not a mystery: elapsed
time went to on-CPU work (a small slice) and off-CPU waiting (the rest),
and the machine-wide CPU utilization graph only ever reported the first
part.

## Technical Explanation

"The CPU is busy" and "the workload is CPU-bound" are not the same claim,
and this chapter is only building the vocabulary to keep them apart — the
full mechanism (thread states, the scheduler, run queues) is Part V's
subject, not this chapter's. For now, what matters is this: machine-wide
CPU utilization is an *average over every core and every thread on the
box*. A handful of threads on the critical path of a single slow request
can each spend most of their time off-CPU (blocked on a lock, waiting on
a network reply, waiting for a disk write to complete) while the rest of
the machine's cores handle unrelated work. The average across all of that
activity can easily land at 20%, even though the *specific* threads a
user is waiting on are doing almost nothing but waiting, one after
another, for their whole slice of the request.

This is also why "add more machines" didn't help the pricing API: extra
machines add more spare CPU capacity, but the slow request's critical path
wasn't limited by CPU capacity in the first place. A bottleneck that is a
lock, a downstream service, or a disk doesn't shrink because idle CPUs
exist elsewhere.

## Tool View

At this stage, before any specialized profiling tool has been introduced,
the smallest useful measurement is comparing **wall time** (elapsed
real-world time) against **user time** and **system time** (CPU time
actually spent executing, split between a process's own code and time
spent in the kernel on its behalf):

- What is measured: how much wall-clock time passed, and how much of that
  time the CPU was actually retiring this process's instructions (user)
  versus doing kernel work on its behalf (system).
- What is not measured: *why* any gap between wall time and CPU time
  exists — this comparison can tell you a gap is there, not what's inside
  it. Identifying the specific cause (a lock, a syscall, a sleep) needs
  tools introduced in later chapters.
- Required permissions: none beyond running the program.
- Likely overhead: negligible — this is just reading numbers the OS
  already tracks.
- Portability: `/usr/bin/time -v` (GNU time) reports the fullest
  breakdown and is common on Linux; macOS's built-in `/usr/bin/time`
  doesn't support `-v`, so this chapter's lab falls back to the shell's
  built-in `time`, which reports the same three core numbers (real, user,
  sys) everywhere.
- Common failure mode: reading "low CPU%" as "nothing to investigate."
  That reading is exactly the mistake this chapter exists to correct.

## Guided Lab

**Portability:** portable — no root, no `perf`, no special hardware.

**Setup:** build `cyclelab` if you haven't already:

```bash
make lab-cyclelab   # from the repo root
```

**Command 1 — see what's not built yet.** Run the `mixed` mode this
chapter's story is really about:

```bash
./labs/cyclelab/bin/cyclelab mixed
```

This prints `mixed: not yet implemented` to stderr and exits with code 2.
That's expected — `mixed` (a workload that alternates compute, locking,
and waiting, exactly like the pricing API story) is one of `cyclelab`'s
planned modes, not yet built (see `labs/cyclelab/README.md`'s mode-status
table). It's worth running once anyway: the CLI surface for it already
exists, and a future chapter will come back to it.

**Command 2 — the fallback, which is built:**

```bash
./labs/scripts/ch1_time_accounting.sh 2 2
```

This runs `cyclelab compute --duration=2 --threads=2` under `time` and
prints wall/user/sys time.

**Fallback path:** Command 2 above *is* this chapter's fallback path —
since `mixed` mode isn't built yet, `cyclelab compute` under `time` is
what actually demonstrates the time-accounting split this chapter's
story depends on, and it needs nothing `mixed` mode would have needed
either (no root, no special hardware).

**Expected qualitative result:** with 2 threads each kept fully busy for 2
seconds, wall time should land close to 2 seconds, while user time should
land close to *twice* that — roughly 4 seconds — because two CPUs' worth
of work happened during those 2 seconds of wall-clock time. System time
should be small. This is the opposite direction from the pricing API
story (here, CPU time exceeds wall time because of true parallelism,
rather than wall time exceeding CPU time because of waiting) — seeing
both directions is the point: elapsed time alone doesn't tell you which
one you're looking at.

**Interpretation:** if you predicted, in the Predict Before Measuring
section, that a program half-blocked on I/O would show roughly 50% CPU
usage while it ran — that's the same accounting, seen from the other
side. A single thread that's on-CPU half the time and off-CPU half the
time contributes about half of one CPU's worth of "busy" to whatever is
averaging it. Neither number, by itself, says whether that's a problem.

**Cleanup:** nothing to clean up; the script doesn't leave anything
running.

## Common Misconceptions

**M01 — "Low average CPU usage means the CPU cannot be involved in
latency."** This is wrong because a single slow request can be limited by
one or two threads on its critical path, each mostly off-CPU, while the
rest of the machine's cores stay busy with unrelated work — the average
hides exactly the threads you care about. The evidence that distinguishes
the two: compare the wall time of the *specific* slow request's critical
path against machine-wide utilization, not against each other as if they
answered the same question.

**M02 — "100% CPU means a process is CPU-bound in the useful-work
sense."** This is wrong because a CPU reporting "busy" can be spinning,
retiring wasted speculative work, handling kernel bookkeeping, or stalled
waiting on memory while technically not idle. The evidence that
distinguishes the two: compare completed work per CPU-second (e.g.
`cyclelab`'s `throughput_ops_per_s`) across two runs that both report
100% CPU — if one does far less useful work per busy CPU-second than the
other, "100% busy" was not "100% useful."

## Practical Implications

Before opening any profiler, the first move in a real investigation is to
ask which of on-CPU time and off-CPU time actually dominates the *specific
slow thing* you care about — not the machine as a whole. That single
question determines almost everything about which tool is worth reaching
for next: CPU profiling tools (Part III) only illuminate on-CPU time; a
workload dominated by off-CPU waiting needs the tools Part VI eventually
introduces instead. Getting this wrong first — assuming CPU-bound when the
real bottleneck is off-CPU, or vice versa — is the single most common way
a performance investigation wastes a day.

## Key Takeaway

**A slow workload is an accounting problem: the first task is to explain
where its elapsed time went.**

## What to Remember

- Elapsed time is only ever on-CPU time plus off-CPU time — there is
  nowhere else for it to go.
- Machine-wide CPU utilization is an average; it can stay low while the
  specific threads on a slow request's critical path are the bottleneck.
- A bottleneck is whatever currently limits the workload, not whatever is
  easiest to see on a dashboard.
- "100% busy" and "doing useful work" are different claims about a CPU.
- Comparing wall time against user/system time is the cheapest way to
  notice a gap exists, even before you can explain it.
- Adding more machines does not help a bottleneck that isn't CPU capacity.
- The critical path — the specific chain of dependent waits — is what
  determines a single request's minimum possible time, not the busiest
  resource on the box.

## Further Reading

- Brendan Gregg, *Systems Performance: Enterprise and the Cloud*, 2nd ed.
  (Addison-Wesley) — the USE Method chapter is the standard reference for
  moving past "check CPU%" as a diagnostic habit. See `references/bibliography.md`.
- Linux `perf` documentation: <https://docs.kernel.org/admin-guide/perf/index.html>
  (not yet needed for this chapter's lab, but the destination this line of
  questioning eventually leads to, starting in Chapter 10).

## The Next Obvious Question

What exactly does "faster" mean for this workload?
