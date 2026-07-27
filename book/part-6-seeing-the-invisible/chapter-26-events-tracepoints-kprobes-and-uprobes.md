# Events, Tracepoints, Kprobes, and Uprobes

**Part:** Part VI — Seeing the Invisible
**Concept level:** 6
**Prerequisites:** counter, sampling, tracing (Chapter 11); process, thread (Chapter 1)
**New concepts:** event, tracepoint, function entry/return, kprobe, kretprobe, uprobe, USDT, argument capture, event rate

## Opening Question

What can dynamic tracing observe that counters and sampling cannot?

## Incident or Real-World Story

A lookup service reports a comfortable p50 and an unremarkable average
latency, but a small, stubborn fraction of requests — well under 1% —
take over 200ms instead of the usual 2ms. `perf stat` shows nothing
unusual in the aggregate counters. A `sample`-style profile, captured
over several seconds, shows the expected hot path and nothing else — at
a sub-1% occurrence rate, a periodic sampler is statistically unlikely
to land a sample inside the rare slow calls at all, and even if it does,
one sample among thousands tells you almost nothing about *how often*
this happens, *how long* it lasts, or what the *slow* calls have in
common that the fast ones don't.

The team's actual question isn't "where does this program spend its
time" — Chapters 11-15 already answer that well. It's "of the specific
function that handles this lookup, what is the full distribution of its
individual call durations, and what do the slow ones have in common?"
That's a different question, and it needs a different kind of
instrument: not a periodic photograph of where execution happens to be,
but something that fires exactly when a specific, named thing happens —
this function starting, this function returning — every single time,
capturing exactly the value that distinguishes a fast call from a slow
one.

## Predict Before Measuring

Before reading further: if the slow call happens roughly 1 time in
2,000, and a sampling profiler captures a stack 99 times per second,
roughly how many minutes would you need to sample continuously before
you'd expect to have captured even a handful of the slow calls'
stacks — and even then, would you know which captured samples were
*from* a slow call versus an ordinary one that just happened to be
running at that instant?

## Worked Example

Suppose the suspect function is called exactly 50,000 times over some
measurement window, and 24 of those calls (0.048%) take over 200ms. A
plain event *count* — "this function was called 50,000 times" — is
trivial to get from existing counters, and it tells you nothing about
the 24 outliers: not their duration, not their frequency over time, not
what request parameters they shared. To get the *distribution* of
individual call durations, you need a timestamp recorded at function
entry and another at function return, for the same call, so the two can
be subtracted — which means attaching measurement code to two specific
events (entry and return) for one specific function, not to the
function's call count in the aggregate. This is exactly the gap between
Chapters 1-25's model of the system (workloads, CPU work, memory,
scheduling, topology) and what this chapter introduces: a way to attach
measurement to a *named event* rather than to a periodic sample or a
running total.

## Core Intuition

An **event**, in this chapter's sense, is a specific, nameable thing
that happens during execution — a function being entered, a function
returning, a system call being made, a scheduler decision being taken —
that can be given a fixed point to attach instrumentation to. A
**tracepoint** is an event location the kernel's own developers
deliberately built in and documented, with a stable name and a defined
set of arguments — the kernel equivalent of a designed API, not an
incidental implementation detail. A **kprobe** attaches to the entry of
almost any kernel function by name, even ones that were never designed
to be probed; a **kretprobe** is the matching mechanism for a function's
*return*, which is exactly what makes entry-to-return duration
measurable at all. A **uprobe** is the same idea applied to user-space
code — attaching to almost any function in a running binary or shared
library. **USDT** (userland statically-defined tracing) is to
user-space uprobes roughly what a tracepoint is to a kprobe: a
deliberately placed, named, documented probe point that the
application's own authors built in on purpose, rather than an arbitrary
function address. Once a probe fires, **argument capture** is what
makes it useful — reading the specific values present at that instant
(a function's arguments, a return value, a timestamp) rather than just
noting that the event happened. **Event rate** is how often a given
probe actually fires per unit time in practice, which matters because
it is the single biggest lever on how much overhead attaching that
probe will cost.

## Technical Explanation

The four probe types differ mainly in *stability* and *where* they
attach, not in what they let you do once attached. Tracepoints and USDT
probes are the stable end of the spectrum: their names and argument
sets are part of a deliberate, documented interface, so a script written
against one is far more likely to keep working across a kernel or
application upgrade. Kprobes and uprobes are the flexible end: they can
attach to nearly any function by symbol name, including internal
functions nobody designed as a public interface, which means a kernel
or library upgrade that renames, inlines, or restructures that function
can silently break the probe — this is exactly the first of this
chapter's mandatory cautions, and it is not a hypothetical: internal
kernel functions genuinely do change between releases in ways tracepoint
names deliberately do not. This is why the practical tool ladder runs in
one direction: reach for an existing, stable tracepoint first; fall back
to USDT if the userland target has one; only reach for a raw function
probe (kprobe/uprobe) when the specific event you need has no stable
name yet — the same "prefer the least invasive tool that answers the
question" discipline Section 15's tool ladder already established for
counting, sampling, and tracing generally.

Argument capture is what turns "this event happened" into a usable
measurement. A kretprobe alone tells you a function returned; capturing
its return value tells you *what* it returned; pairing a kprobe's entry
timestamp with the matching kretprobe's return timestamp gives you that
one call's duration — and doing this for every call, not a sampled
subset, gives you the full distribution the sampling profiler in this
chapter's story couldn't statistically reach. This completeness is
exactly why event rate matters as its own concept: a probe on a function
called once a second costs essentially nothing; the same probe on a
function called a million times a second means a million executions of
whatever work the probe does, on the hot path, every second — the third
mandatory caution. A high-frequency probe is not automatically
disqualifying, but its cost has to be reasoned about before it's
deployed, not discovered afterward as a mysterious slowdown.

## Tool View

- What is measured: the exact arguments, return values, and timing of a
  specific, named event, every time it fires — not an aggregate count
  and not a statistical sample.
- What is not measured: anything that never triggers a probed event;
  correlating two separate probes (e.g., matching an entry to its
  return, or one thread's request to another's response) requires
  deliberate design, not something the mechanism gives you for free.
- Required permissions: kprobes, kretprobes, and uprobes typically
  require root or specific elevated capabilities on Linux, and are
  further gated by `perf_event_paranoid` (Chapter 10) and
  `kptr_restrict` (Chapter 13); tracepoints are generally more broadly
  accessible but still commonly restricted in practice. This is the
  fourth mandatory caution in a different form: **data captured this way
  can include another process's arguments — file paths, buffer
  contents, addresses — so the same access control that protects a
  process's memory generally also gates who can attach probes to it.**
- Likely overhead: roughly proportional to event rate times the work
  done per event (argument capture, timestamping, any filtering) — the
  same product this chapter's Technical Explanation already introduced,
  and the reason "trace everything, figure it out later" is not a free
  default strategy (the caution behind this chapter's Common
  Misconceptions section below).
- Portability: **Linux-only as designed** — kprobes, kretprobes,
  uprobes, and USDT are Linux kernel/tracing-infrastructure concepts
  with no direct equivalent on other operating systems. The closest
  conceptual relative on this book's own macOS reference machine is
  **dtrace**, Sun/Solaris-derived dynamic tracing shipped with macOS,
  which offers an analogous probe-provider model (kernel and user-space
  probes, argument capture, a scripting language to act on them) but is
  a genuinely different implementation with different probe names,
  different syntax, and — on a System Integrity Protection-enabled Mac
  — different privilege requirements, confirmed directly below.
- Common failure mode: a kprobe or uprobe that attached cleanly on one
  kernel or binary version silently fails to attach (or attaches to the
  wrong thing) after an upgrade, because the internal function it named
  was renamed, inlined away, or restructured — the first mandatory
  caution again, showing up as a debugging problem instead of an
  abstract warning.

## Guided Lab

**Portability:** **privileged** (Linux tracepoints/kprobes/kretprobes/
uprobes generally require root or elevated capabilities) and, on this
book's own macOS reference machine, **not available unprivileged at
all** — confirmed directly below, not assumed.

**Setup:** none beyond a working `cyclelab` build (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch26_probe_availability.sh
```

**Expected qualitative result:** on this book's reference machine, the
script's real, tested output shows `dtrace` present but refusing to list
probes without elevated privileges (`dtrace: failed to initialize
dtrace: DTrace requires additional privileges`), and `bpftrace` absent
entirely, since it is Linux-only. `make doctor`'s own "dtrace" and
"bpftrace / BCC" sections report the same two facts independently, using
the same real commands.

**Interpretation:** this is not a failure of the lab — it *is* the
lab's result, and it is real, reproducible evidence for two of this
chapter's mandatory cautions at once: dynamic tracing commonly requires
privilege (because it can observe another process's arguments), and
that requirement is not uniform across platforms or even across security
configurations of the same platform. On genuine Linux hardware with root
or `CAP_SYS_ADMIN`/`CAP_BPF`, the schematic commands below are what the
same investigation looks like — **documented against each tool's stable
interface, not tested against real captured output on this reference
machine**:

```bash
# List available tracepoints (stable names, no kernel-internal knowledge needed)
sudo cat /sys/kernel/debug/tracing/available_events | grep sched

# Attach a dynamic probe to a specific kernel function's entry and return,
# turning it into a tracepoint-like event perf can record
sudo perf probe --add do_sys_openat2
sudo perf probe --add 'do_sys_openat2%return $retval'
sudo perf record -e probe:do_sys_openat2 -e probe:do_sys_openat2__return -aR sleep 5
sudo perf script

# Remove the dynamic probes when finished
sudo perf probe --del do_sys_openat2*
```

**Fallback path:** this chapter's real, tested content *is* the
availability check above, plus everything Chapters 11-25 already
established about counters, sampling, and stack capture — Chapter 29
extends `sample`'s own wall-clock, all-thread-state capture (already
demonstrated in Chapters 11-15) into a genuine, tested off-CPU
observation, without requiring Linux's dynamic-tracing infrastructure at
all.

**Cleanup:** none.

## Common Misconceptions

### *"More tracing produces more truth." (M14)*

**Why it's wrong:** Event volume itself has a cost and a
signal-to-noise budget: a probe that fires constantly can perturb the
very workload it's measuring (the overhead this chapter's Tool View
section quantifies as rate times per-event work) and can bury the
specific signal you actually wanted under an overwhelming volume of
routine, uninteresting events.

**Correct intuition:** The same tool-selection discipline Section 15
already established — pick the least invasive instrument, and the
narrowest, most specific event, that actually answers the question in
front of you; a kprobe on every function call in a hot loop is rarely
the right first move even when it's technically possible. Compare a
narrowly scoped probe (one function, one specific condition) against an
unfiltered, everything-that-moves trace of the same workload — the
unfiltered version costs more overhead, produces far more data to sift
through, and answers the original question no better.

**Analogy:** Wiretapping every phone in a building doesn't help you
find one specific conversation — it buries it under thousands of
irrelevant calls you now have to sift through, when a single, targeted
line would have answered the question directly.

### *"Sampling profiles show all latency." (M08, revisited)*

**Why it's wrong:** Chapter 12 introduced this misconception in the
context of on-CPU profiling; this chapter's own opening story is a
second, sharper instance of the same gap — a rare, brief event can be
statistically almost invisible to a periodic sampler no matter how
faithfully that sampler represents *typical* execution.

**Correct intuition:** Dynamic tracing's fundamental advantage over
sampling is exactly here: it does not depend on statistically colliding
with the event of interest, because it fires deterministically every
time that event occurs.

**Analogy:** A security guard doing hourly rounds might never happen to
walk past the one door that's briefly propped open at 3:14am — a sensor
wired directly to that door catches it every single time, because it
doesn't depend on timing a glance correctly.

## Practical Implications

Reach for a stable tracepoint or USDT probe before a raw kprobe/uprobe
whenever one exists for the event you need — the same investigation
answered by either will keep working across upgrades in one case and
may silently break in the other. Before attaching any high-frequency
probe, estimate its event rate first (a rough call-count check is often
enough) and reason about overhead before deploying it broadly, rather
than discovering the cost as an unexplained slowdown afterward. Treat
captured arguments as potentially sensitive from the start — a probe
that captures function arguments from another process is, by
construction, capturing that process's data, which deserves the same
handling care as logs or any other observability data that might touch
credentials, file contents, or personal information.

## Key Takeaway

**Dynamic tracing attaches measurement to specific events, allowing
questions that aggregate counters and periodic samples cannot answer.**

## What to Remember

- An event is a specific, nameable thing that happens during execution;
  dynamic tracing attaches measurement directly to it, rather than
  sampling periodically or accumulating a running total.
- Tracepoints (kernel) and USDT (user space) are stable, deliberately
  designed probe points; kprobes/kretprobes and uprobes attach to nearly
  arbitrary function entry/return points, more flexible but more
  fragile across version changes.
- A kretprobe's return-time timestamp, paired with the matching probe's
  entry-time timestamp, is what turns "this function was called" into
  "here is this call's actual duration" — an entry-count alone cannot
  give you a duration distribution.
- Event rate times per-event work is the core overhead equation for any
  probe; reason about it before deploying broadly, not after.
- Dynamic tracing commonly requires elevated privileges, because it can
  observe another process's arguments — treat captured data with the
  same care as any other sensitive observability data.
- On this book's own macOS reference machine, neither dtrace
  (privilege-restricted) nor bpftrace (absent) is usable unprivileged —
  a real, tested limitation, not a hypothetical caveat.

## Further Reading

- Linux tracepoints documentation: <https://docs.kernel.org/trace/tracepoints.html>
- Linux kprobes documentation: <https://docs.kernel.org/trace/kprobes.html>
- Linux uprobe-tracer documentation: <https://docs.kernel.org/trace/uprobetracer.html>

## The Next Obvious Question

How can eBPF safely run custom measurements inside the kernel?
