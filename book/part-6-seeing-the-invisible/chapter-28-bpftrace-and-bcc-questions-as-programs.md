# Chapter 28 — `bpftrace` and BCC: Questions as Programs

**Part:** Part VI — Seeing the Invisible
**Concept level:** 6
**Prerequisites:** eBPF program, hook, verifier, helper, map, per-CPU map, ring buffer, BTF, CO-RE (Chapter 27)
**New concepts:** probe specification, predicate, action, aggregation, histogram, stack aggregation, interval output

## Opening Question

How do `bpftrace` and BCC turn questions into live instrumentation?

## Incident or Real-World Story

An on-call engineer, mid-incident, needs an answer in the next two
minutes, not after writing and compiling a libbpf application against
Chapter 27's full execution model by hand. "Which process is calling
this syscall the most, right now, on this specific machine" is exactly
the kind of question eBPF can answer efficiently — but only if there's a
way to state it that's faster to write than the incident itself. This is
the gap `bpftrace` and BCC exist to close: turning Chapter 27's verifier,
hooks, maps, and helpers into something closer to a one-line question
than a compiled program.

## Predict Before Measuring

Before reading further: given everything Chapter 27 established about
maps and aggregation happening inside the kernel, do you expect a tool
like `bpftrace` to need its own compiler step for every new question, or
do you expect something closer to a small, interpreted scripting
language sitting on top of the same verified-program machinery?

## Worked Example

The question "which processes call this syscall most?" decomposes
directly into Chapter 27's model: the hook is the syscall's entry point
(a tracepoint, per Chapter 26's stability preference); the aggregation
key is the calling process's name or ID; the action is incrementing a
per-key counter in a map; and the output, read once at the end (or
periodically), is that map's contents sorted by count. A `bpftrace`
one-liner for exactly this shape looks like:

```
bpftrace -e 'tracepoint:syscalls:sys_enter_read { @[comm] = count(); }'
```

Read as English: at the `sys_enter_read` tracepoint, for the calling
process's name (`comm`), increment a counter. Nothing here mentions the
verifier, a map type, or a hook registration call directly — `bpftrace`
generates all of that from this one line, which is precisely what makes
it fast enough to type mid-incident.

## Core Intuition

A **probe specification** names the hook a piece of logic attaches to —
`tracepoint:syscalls:sys_enter_read` in this chapter's Worked Example,
or `kprobe:some_function`, following Chapter 26's own event-naming
vocabulary directly. A **predicate** is an optional filter — run this
logic only when some condition holds, so the aggregation only sees the
events that actually matter to the question being asked. An **action**
is the logic that runs when the probe fires and its predicate (if any)
passes — most commonly updating an **aggregation**, `bpftrace`'s
built-in map-like construct (the `@` variables in this chapter's
example) that handles the key/value bookkeeping Chapter 27's maps
provide underneath. A **histogram** is a specific, extremely common
aggregation shape — bucketing a value's distribution rather than just
summing or counting it, built directly on eBPF's map machinery. A
**stack aggregation** counts by *call stack* as the key instead of a
process name or an ID, turning "which code paths call this the most"
into the same count-grouped-by-key shape as any other aggregation.
**Interval output** prints an aggregation's current state on a fixed
schedule (once a second, for instance) instead of only once at the very
end, useful for watching a live system rather than summarizing a fixed
window after the fact.

## Progression

1. list available probes;
2. count an event;
3. group by process or thread;
4. measure a duration;
5. build a histogram;
6. capture user or kernel stacks;
7. use a packaged BCC tool before writing a custom script.

This is a genuine escalation in cost and specificity, not an arbitrary
checklist: each step adds either more precision (grouping, duration,
stacks) or more packaging (a maintained tool instead of a hand-rolled
script), and the right stopping point is wherever the question is
already answered — the same "least invasive tool that answers the
question" discipline Section 15 established from the very first chapter
that needed a tool at all.

## Example questions

- Which processes call this syscall most?
- What is the distribution of block-I/O latency?
- How long do threads wait to be scheduled after wake-up?
- Which user-space stacks allocate memory?
- Which locks or futex waits dominate blocked time?

Every one of these is a probe specification plus a key plus an
aggregation shape, exactly this chapter's Worked Example's decomposition
applied to a different hook and a different key.

## Technical Explanation

`bpftrace` compiles its small scripting language down to eBPF bytecode
at invocation time, submits it through Chapter 27's verifier like any
other eBPF program, and manages attaching it to the named hook,
collecting aggregation output, and tearing it down when the script
exits — all of Chapter 27's execution model, with the compilation and
loader boilerplate hidden behind a much shorter surface syntax. BCC
takes a related but distinguishable approach: rather than a standalone
scripting language, it provides a Python (or Lua) front end and a
library of pre-built, commonly needed tracing programs — `execsnoop`,
`biolatency`, `runqlat`, and dozens more — each one a maintained,
packaged answer to one of this chapter's Example Questions, ready to run
without writing any new probe logic at all. This is precisely step 7 of
this chapter's Progression: a packaged BCC tool has already made the
predicate, key, and aggregation design decisions a hand-rolled script
would otherwise require, and reusing it is both faster and less
error-prone whenever the existing tool already answers the question in
front of you.

## Tool choice policy

- use existing BCC tools for standard investigations;
- use `bpftrace` for short, inspectable questions;
- use libbpf/CO-RE for maintained production tooling;
- explain that package names and tool suffixes vary by distribution.

## Guided Lab

**Portability:** **privileged** and **Linux-only** — neither `bpftrace`
nor BCC exists on this book's own macOS reference machine at all
(confirmed directly by Chapter 26's `ch26_probe_availability.sh` and by
`make doctor`'s own "bpftrace / BCC" section, both reporting the tools
absent, not merely restricted).

**Setup:** a working `cyclelab` build (`make lab-cyclelab`); no
`bpftrace`/BCC install is possible to arrange as part of this lab on
this reference machine.

**Command:**

```bash
./labs/scripts/ch28_manual_aggregation.sh
```

**Expected qualitative result:** real, reproducible output showing the
same **count-grouped-by-key** and **histogram** shapes this chapter's
Core Intuition describes, computed by hand from `cyclelab lock-
contention`'s own JSON output rather than live inside a kernel. Running
it twice produces different exact numbers (natural scheduling variance,
Chapter 21) but the same qualitative shape: ten threads' increment
counts landing in a fairly narrow band, with a roughly bell-shaped
histogram rather than a sharply bimodal one, since ten equal-priority
threads contending one mutex tend to get roughly, not exactly, equal
turns.

**Interpretation:** this script deliberately does **not** simulate
`bpftrace` or claim to be a substitute for it — it demonstrates that
**aggregation is a general concept** (reduce many events down to counts,
sums, or distributions keyed by something meaningful), while what
`bpftrace`/eBPF specifically add is *where* that aggregation happens (in
kernel, at the event itself, avoiding a per-event round trip to user
space, exactly Chapter 27's Worked Example) and *what events* it can
attach to (Chapter 26's kernel- and user-space probes). The concept
transfers even where the mechanism cannot.

On genuine Linux hardware with root access and `bpftrace` installed, the
following one-liners are what the same underlying questions look like
using the real tool — **documented against `bpftrace`'s stable syntax,
not tested against real captured output on this reference machine**:

```bash
# Count syscall calls grouped by process name (this chapter's Worked Example)
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_read { @[comm] = count(); }'

# Histogram of a duration (measured between two probes)
sudo bpftrace -e '
kprobe:vfs_read { @start[tid] = nsecs; }
kretprobe:vfs_read /@start[tid]/ {
    @latency_ns = hist(nsecs - @start[tid]);
    delete(@start[tid]);
}'

# Stack aggregation: which user-space stacks allocate memory most often
sudo bpftrace -e 'uprobe:libc:malloc { @[ustack] = count(); }'

# A packaged BCC tool, no custom script needed (Progression step 7)
sudo biolatency 5 1
sudo runqlat 5 1
```

**Fallback path:** this chapter's real, tested content *is* the manual
aggregation script above, plus Chapter 26's real, tested probe-
availability check and every stack-capture and JSON-aggregation tool
this book has already built and tested through Chapter 27.

**Cleanup:** none.

## Common Misconceptions

**More tracing produces more truth (M14, revisited).** A `bpftrace`
one-liner with no predicate at all — tracing every call to a
high-frequency function, unfiltered — reintroduces exactly Chapter 26's
overhead and signal-to-noise caution, just with a shorter syntax for
writing it. A predicate that narrows the probe to the specific condition
under investigation is usually the difference between a useful,
inspectable one-liner and an accidentally expensive, noisy one.

**Aggregation, counting, and histograms require eBPF or a kernel-level
tracer (M43).** This overstates what's actually special about eBPF's
version of aggregation. Counting events grouped by a key, and bucketing
values into a histogram, are general data-reduction operations that can
be computed anywhere data exists — including, as this chapter's own
Guided Lab demonstrates, entirely in user space, after the fact, from
ordinary JSON output. What eBPF specifically contributes is aggregating
*at the event itself, inside the kernel*, avoiding a per-event
user-space round trip (Chapter 27's Worked Example) — a real efficiency
and completeness advantage, but not a claim that the underlying
arithmetic of "count grouped by key" is otherwise impossible.

## Practical Implications

Start from this chapter's Example Questions list, or something shaped
like it, before reaching for a specific tool — the probe specification,
key, and aggregation shape usually falls out directly once the question
is stated precisely enough. Prefer a packaged BCC tool over a
hand-written `bpftrace` script whenever one already exists for the
question at hand; reach for `bpftrace` for a short, inspectable,
one-off question; reserve libbpf/CO-RE for tooling meant to run
continuously in production, where its lower per-invocation overhead and
maintainability matter more than one-liner brevity. Always add a
predicate to narrow scope before running anything unfiltered against a
high-frequency hook in a system that matters.

## Key Takeaway

**`bpftrace` and BCC are most powerful when the question is stated
precisely enough to become an event, a key, and an aggregate.**

## What to Remember

- A probe specification names the hook (Chapter 26's tracepoints,
  kprobes, kretprobes, uprobes); a predicate narrows which firings
  matter; an action updates an aggregation.
- Aggregations, histograms, and stack aggregations are all the same
  underlying map-based mechanism (Chapter 27) applied to different keys
  and different value shapes.
- `bpftrace` compiles a short scripting language down to verified eBPF
  bytecode at invocation time; BCC provides a Python/Lua front end plus
  a library of maintained, packaged tools for common questions.
- Counting and histogramming are general concepts, not eBPF-exclusive
  ones — what eBPF specifically adds is doing that aggregation at the
  event, inside the kernel, without a per-event user-space round trip.
- Prefer a packaged BCC tool, then a short predicated `bpftrace`
  one-liner, then libbpf/CO-RE for production tooling — in that order,
  matching the cost and specificity each step actually requires.
- Neither tool is available, even in principle, on this book's own
  macOS reference machine — real, tested absence, not a hypothetical
  caveat.

## Further Reading

- bpftrace documentation: <https://bpftrace.org/docs/release_026/docs>
- BCC project and tools: <https://github.com/iovisor/bcc>

## The Next Obvious Question

Where does time go when a thread is not on a CPU?
