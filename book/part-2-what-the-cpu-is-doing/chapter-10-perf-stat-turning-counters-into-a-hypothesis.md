# `perf stat`: Turning Counters into a Hypothesis

**Part:** Part II — What the CPU Is Doing
**Concept level:** 2-3 (bridges into Level 3 "counter" from Section 11; full Level 3 treatment begins Part III)
**Prerequisites:** cycle, instruction count, IPC, CPI, front end, back end, stall, dependency, branch, misprediction (Chapters 6-9)
**New concepts:** PMU, hardware event, software event, event group, multiplexing, scaling, per-thread versus system-wide measurement, privilege restrictions

## Opening Question

How can counters turn a vague slowdown into a hypothesis?

## Incident or Real-World Story

An engineer is told a service "got slower" after a deploy and, remembering
Chapters 6 through 9, has real vocabulary now for what that could mean:
maybe the new code has a longer dependency chain, maybe it introduced an
unpredictable branch, maybe instruction count went up for an unrelated
reason. Rather than reasoning about which of these it is from the source
diff alone, they reach for `perf stat` — not to search blindly through
every counter the CPU offers, but to check a short, deliberately chosen
list: task-clock, cycles, instructions, and, once those raise a specific
question, one or two more targeted events. Ten minutes later, they have
an actual answer: instructions per record barely changed, but cycles per
record went up substantially — pointing straight at something *between*
instructions, not the instructions themselves, which narrows the next
step to Chapter 8 and 9's territory (dependencies or branches) rather
than Chapter 6's (what got compiled).

This is the payoff of Chapters 6 through 9 existing before this one:
`perf stat`'s numbers only became a hypothesis instead of a wall of
digits because the engineer already had a mental model for what cycles,
instructions, and their ratio could mean.

## Predict Before Measuring

Before reading further: if you ran `perf stat` on the exact same command
five separate times, would you expect its reported `instructions` count
to vary run to run? Would you expect its reported `cycles` count to vary?
Which one do you predict is more consistent, and why — think back to
Chapter 4's variance discussion and Chapter 7's frequency-scaling caution
before answering.

## Core Intuition

A **PMU** (Performance Monitoring Unit) is dedicated hardware inside the
CPU that counts specific architectural events — instructions retired,
cycles elapsed, cache misses, branch mispredictions — without slowing
down the code being measured to do so. A **hardware event** is something
the PMU itself counts directly (cycles, instructions, cache references);
a **software event** is something the kernel counts on the CPU's behalf
(context switches, page faults) using a different mechanism entirely,
even though `perf stat` presents both through the same interface. An
**event group** is a set of events `perf stat` is asked to count
together, guaranteed to be measured over the exact same interval — which
matters because the PMU has a limited number of physical counter
registers, and asking for more events than there are registers forces
**multiplexing**: the kernel time-slices the requested events across the
available registers and **scaling** extrapolates full-run estimates from
the fraction of time each event was actually being counted. `perf stat`
can measure a single process/thread or run **system-wide** (`-a`,
across every CPU) — a different, much broader question than "how did
this one program run."

## Technical Explanation

The interpretation workflow this chapter teaches is deliberately ordered,
and the order matters more than any individual step:

1. **Verify elapsed and CPU time first.** Before reading a single
   specialized counter, confirm the run itself looks like what you
   expect — Chapter 1's wall/user/system-time comparison, still the
   foundation.
2. **Inspect context switches and migrations.** A run with unusually high
   context-switch or migration counts (software events `perf stat`
   reports by default) may be dominated by scheduling noise, not by
   anything in the code — a confounder worth ruling out before trusting
   any CPU-level counter (Chapter 4's discipline, applied to counter
   output specifically).
3. **Inspect instructions and cycles**, and their ratio — Chapter 7's
   IPC/CPI, now read from a real tool instead of inferred from
   throughput.
4. **Add only events relevant to the current hypothesis.** Requesting
   every event `perf list` offers "just in case" invites multiplexing
   (see below) and produces a wall of numbers with no organizing
   question behind it — the opposite of what this chapter is teaching.
5. **Notice unsupported or multiplexed counters.** `perf stat`'s output
   marks scaled/multiplexed results explicitly; treat a heavily-scaled
   number (measured for a small fraction of the run) with proportionally
   less confidence.
6. **Never read a generic `cache-misses` event as a complete cache
   diagnosis.** One aggregate counter cannot distinguish an L1 miss that
   hit L2 immediately from a full miss all the way to memory — that
   distinction needs the tools Part IV introduces.

### Core command progression

```bash
# The simplest possible measurement: one run, default event set.
perf stat -- ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=1

# Repeat it, per Chapter 4's hygiene -- perf stat can do the repetition
# and report variance itself.
perf stat -r 10 -- ./labs/cyclelab/bin/cyclelab compute --duration=2 --threads=1

# A specific, hypothesis-driven event set (Chapter 9's branch-prediction
# question, measured directly instead of inferred from throughput).
perf stat -e cycles,instructions,branches,branch-misses -- \
  ./labs/cyclelab/bin/cyclelab branch --duration=2 --threads=1 --pattern=random

# System-wide measurement over a fixed duration -- a different question
# ("what did the whole machine do for 10 seconds") than any of the above.
perf stat -a -- sleep 10
```

**These commands are documented, not tested against real output** — the
reference machine for this book is macOS, where `perf` does not exist
(confirm this for yourself with `make doctor`, whose "perf" section will
report `[SKIP]` on any non-Linux machine). Their syntax and event names
follow `perf`'s stable, documented interface. A schematic (illustrative,
not captured) example of what the first command's output commonly looks
like on Linux:

```text
 Performance counter stats for './cyclelab compute --duration=2 --threads=1':

          2,000.34 msec task-clock                #    0.999 CPUs utilized
                 3      context-switches          #    1.500 /sec
                 0      cpu-migrations            #    0.000 /sec
        41,234,112,987      cycles                    #    3.100 GHz
        68,912,004,551      instructions              #    1.67  insn per cycle
        ...

       2.001234567 seconds time elapsed
```

The specific numbers here are invented for illustration; do not treat
them as anything other than a shape to expect. A real run's task-clock
should land close to the `--duration` requested, cycles should scale
with reported GHz roughly matching the CPU's actual clock, and the
`insn per cycle` line is exactly Chapter 7's IPC, read directly instead
of inferred.

## Tool View

- What is measured: whichever events are requested, over the scope
  requested (one process/thread, or system-wide).
- What is not measured: `perf stat` counts; it does not show *where in
  the code* those events occurred — that's `perf record`'s job, starting
  Chapter 12.
- Required permissions: reading hardware counters typically requires
  either root, or a sufficiently permissive
  `/proc/sys/kernel/perf_event_paranoid` value, or the `CAP_PERFMON`
  capability granted specifically to the user/process (the modern,
  narrower alternative to running as root; check your distribution's
  documentation for how to grant it). `kptr_restrict` separately affects
  whether kernel symbols resolve in any tool that needs them — not
  `perf stat`'s counter output directly, but relevant to `perf record`
  later. **Do not default to "run everything as root"** — narrower
  permission grants exist and are worth using deliberately.
- Likely overhead: generally small for counting (as opposed to
  sampling), but not zero — repeated, controlled runs (Chapter 4) are
  still the right way to trust a `perf stat` number, not a single
  invocation.
- Portability: Linux-only. Exact available events, their names, and PMU
  capabilities vary by CPU vendor and generation — Appendix C is where
  that variance is catalogued; this chapter's main prose intentionally
  stays event-name-agnostic beyond the small, stable set used above.
- Common failure mode: requesting a long list of events at once,
  triggering multiplexing, then treating every reported number as
  equally trustworthy regardless of how much of the run each was
  actually measured for.

## Guided Lab

**Portability:** hardware-dependent / privileged — requires Linux and
either root or a permissive `perf_event_paranoid`/`CAP_PERFMON` setting.
Run `make doctor` first; its "perf" and "perf_event_paranoid" sections
will tell you directly whether this lab is runnable as-is on your
machine.

**Setup (Linux only):**

```bash
make lab-cyclelab   # from the repo root
make doctor          # confirm perf is installed and check perf_event_paranoid
```

**Command:**

```bash
perf stat -r 5 -e task-clock,cycles,instructions -- \
  ./labs/cyclelab/bin/cyclelab compute --duration=1 --threads=1 --chains=1 --quiet
perf stat -r 5 -e task-clock,cycles,instructions -- \
  ./labs/cyclelab/bin/cyclelab compute --duration=1 --threads=1 --chains=8 --quiet
```

**Expected qualitative result:** both runs should report similar
`instructions` counts (same instruction mix, same duration budget scaled
by iteration count — compare `instructions` divided by
`results.total_iterations` from each run's JSON if you redirect
`--output` to a file alongside `perf stat`'s stderr output). The
`--chains=8` run should report a materially higher `insn per cycle`
figure than `--chains=1` — the direct counter confirmation of Chapter
7 and 8's throughput-based evidence for the same effect.

**Interpretation:** if your `perf stat` numbers show `--chains=8` with
meaningfully higher IPC than `--chains=1`, you've directly measured what
Chapters 7 and 8 only measured indirectly through throughput — the same
underlying phenomenon (independent work letting the pipeline stay busier
per cycle), now with the actual hardware counters confirming the
mechanism rather than just its external effect.

**Fallback path:** if `perf` isn't available (macOS, a container without
counter access, a `perf_event_paranoid` setting you can't change), this
chapter's mechanism was already measured indirectly, without any
counter, in Chapters 7 and 8's guided labs — rerun
`./labs/scripts/ch7_ipc_intuition.sh` and treat its throughput
difference as the observable consequence of exactly the IPC difference
this chapter's lab would show directly. What's lost without `perf` is
the direct confirmation, not the underlying evidence.

**Cleanup:** none.

## Common Misconceptions

**M20 (revisited) — "A profiler's output is ground truth."** `perf stat`
is a measurement system with its own scope, overhead, and permission
requirements, same as any tool covered so far — a multiplexed counter's
scaled estimate is exactly the kind of number this misconception (first
named in Chapter 4) warns against trusting at face value. The evidence
that distinguishes the two: check `perf stat`'s own output for scaling
percentages when requesting many events at once, and treat heavily
scaled numbers with proportionally less confidence.

A new, chapter-specific misconception is worth naming directly: **"More
requested events always means a more complete picture."** This is wrong
because the PMU has a limited number of physical counter registers, and
requesting more events than fit forces multiplexing, degrading the
precision of *every* requested event rather than adding a free extra
dimension. The evidence that distinguishes the two: compare a small,
targeted event list's output against a large, unfocused one on the same
command, and check `perf stat`'s reported scaling percentages in the
second case.

## Practical Implications

Before running `perf stat`, decide what hypothesis you're testing —
Chapter 5's discipline, applied to counter selection specifically.
Request the smallest event set that could confirm or refute that
hypothesis, not the largest set available. If a wider sweep is genuinely
useful, run it as a separate, explicit step (fully aware of
multiplexing) rather than defaulting to it as the first move.

## Key Takeaway

**`perf stat` is most useful when a small, hypothesis-driven event set
explains why two controlled runs differ.**

## What to Remember

- The PMU counts hardware events without slowing down the measured code;
  `perf stat` is the standard interface to it on Linux.
- Hardware events (PMU-counted) and software events (kernel-counted) are
  mechanically different but reported through the same `perf stat`
  interface.
- Requesting more events than the PMU has physical counters for triggers
  multiplexing and scaling — treat heavily scaled numbers with less
  confidence.
- The interpretation order matters: elapsed/CPU time, then scheduling
  noise, then instructions/cycles, then hypothesis-specific events —
  not the reverse.
- `perf_event_paranoid`, `kptr_restrict`, and `CAP_PERFMON` govern
  what's measurable without full root; use the narrowest grant that
  works rather than defaulting to root.
- A generic `cache-misses` event is not a complete cache diagnosis —
  Part IV's tools exist because one aggregate counter can't distinguish
  where in the hierarchy a miss actually resolved.
- Everything `perf stat` reports is still subject to Chapter 4's
  benchmarking hygiene — repeat, control, and interleave, rather than
  trusting one invocation.

## Further Reading

- Linux perf documentation: <https://docs.kernel.org/admin-guide/perf/index.html>
- perf tutorial: <https://perfwiki.github.io/main/tutorial/>
- perf manual pages: <https://man7.org/linux/man-pages/man1/perf.1.html>

## The Next Obvious Question

When should we count, sample, or trace?
