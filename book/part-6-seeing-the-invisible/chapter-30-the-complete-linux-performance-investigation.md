# The Complete Linux Performance Investigation

**Part:** Part VI — Seeing the Invisible
**Concept level:** synthesis (draws on Levels 0-7)
**Prerequisites:** the entire book — Chapters 1-29
**New concepts:** none new — formally completes bottleneck shift, regression, scalability, interference, and causal claim, each used operationally since earlier chapters; otherwise synthesizes Chapters 1-29 into one investigation

## Opening Question

How do all the layers combine into one defensible investigation?

## Incident or Real-World Story

A multithreaded service reports acceptable average CPU utilization,
degraded p99 latency, inconsistent benchmark results from run to run,
weak scaling past four workers, and occasional long stalls. Every one of
these symptoms, read in isolation, points somewhere different: average
utilization looking fine suggests the CPU isn't the problem (M01/M02's
territory, Chapters 1 and 21); inconsistent results suggest a
measurement problem (Chapter 4); weak scaling suggests contention or a
shared bottleneck (Chapters 18-19, 22, 29); occasional long stalls
suggest something intermittent that an average would hide entirely
(Chapter 4's variance, Chapter 26's rare-event problem). None of these
symptoms, alone, is the investigation. The investigation is the
disciplined process of working through them in the right order, each
step's evidence narrowing what the next step needs to check — this
book's entire structure, from Chapter 5's investigation loop onward,
compressed into one case study.

## Predict Before Measuring

Before reading further: given only this chapter's five symptoms above,
what is the single first action you would take, and why — and which of
this book's 29 chapters' worth of tools would you deliberately *not*
reach for yet?

## Case study shape

A multithreaded service shows:

- acceptable average CPU utilization;
- degraded p99 latency;
- inconsistent benchmark results;
- weak scaling after four workers;
- occasional long stalls.

The investigation should reveal several layers, but only one at a time:

1. the benchmark is initially invalid because request rate and warm-up
   are uncontrolled;
2. corrected measurements show saturation and tail growth;
3. CPU profiles reveal a broad serialization path;
4. off-CPU tracing shows lock waiting;
5. after fixing contention, throughput rises but scaling stops at
   memory bandwidth;
6. thread placement reveals cross-socket memory traffic;
7. first-touch initialization restores locality;
8. the final benchmark proves improvement and documents remaining
   limits.

## Core Intuition

This chapter also formally completes several terms this book has used
operationally since much earlier chapters, without yet naming them as
their own concept. **Bottleneck shift** — used since Chapter 5's
investigation loop — is exactly what Step 5 demonstrates directly:
removing one confirmed bottleneck (lock contention) reveals the next one
underneath (memory bandwidth), rather than removing the ceiling
entirely. **Regression** is the opposite outcome of a change: a
measured, confirmed *worsening*, checked for with the same before/after
discipline (Chapter 15) that confirms an improvement — the same
methodology, applied to catch the opposite result. **Scalability** is
how a workload's throughput or latency changes as some resource (here,
thread count) increases, exactly what Steps 2 and 5's sweeps each
measure directly, and exactly what "weak scaling after four workers" in
this chapter's own Case Study Shape names as a symptom. **Interference**
— used informally since Chapter 22's own title — is unrelated work
degrading a workload's performance by sharing a contended resource with
it; this case study's own lock contention is a specific, single-process
instance of the same underlying idea Chapter 22 introduced across
separate processes. A **causal claim** — used operationally since
Chapter 5 — is a specific, falsifiable assertion that one particular
change produced one particular measured effect; this chapter's Key
Takeaway is itself a definition of what makes a causal claim
defensible: it must be bounded (about one specific change), and it must
be supported by a measurement chosen to distinguish that explanation
from its plausible alternatives, not just any measurement that happens
to agree with it.

The order of these eight steps is the actual lesson, not just their
content. Each step's evidence is what earns the right to ask the next
step's question — measuring before profiling (Chapters 1-5 before
6-20), profiling before tracing the specific mechanism (Chapters 6-20
before 26-29), fixing the layer the evidence actually points to before
assuming a fix worked (Chapter 15's before/after discipline, M17). A
real investigation almost never confirms its first hypothesis and stops;
it more commonly finds one real bottleneck, fixes it honestly, and
discovers the *next* one underneath — exactly what happens between this
chapter's Steps 4 and 5, and again between Steps 5 and 6.

## Worked Example — this book's own tools, run through the whole shape

**Step 1 — an initially invalid benchmark.** A single, quick measurement
at low, arbitrarily chosen concurrency — this chapter's own lab ran
`cyclelab lock-contention` at exactly 2 threads, once, and got a
perfectly ordinary-looking number. The benchmark isn't invalid because
that number is a lie; it's invalid because testing at one low
concurrency level, once, never asks the question that actually matters
here — what happens as concurrency rises — which is precisely the
uncontrolled-request-rate problem this step names.

**Step 2 — corrected measurement.** Sweeping thread count with three
interleaved repetitions each (Chapter 4's discipline, applied for real):
on this book's reference machine, `lock-contention` throughput measured
186,467 increments/s at 1 thread and just 179,893 at 10 threads —
essentially flat, revealing the saturation and (implicitly, from
run-to-run spread within each thread count) the same kind of tail growth
this chapter's case study describes, invisible to Step 1's single data
point.

**Steps 3-4 — CPU profile and off-CPU tracing.** Chapter 29's own
real, measured result on this exact workload shape: a `sample` capture
of heavily contended `lock-contention` showed 77.8% of sampled stack
frames sitting inside a mutex-wait code path, versus 0.0% for an
equivalent pure-compute capture. This is Steps 3 and 4 together — the
CPU profile alone would already show a suspiciously flat, broad
time distribution across worker stacks (a "broad serialization path"),
and the off-CPU view confirms directly *why*: threads spending most of
their time waiting for one shared lock, not computing.

**Step 5 — fix the contention, hit the next ceiling.** Standing in for
shipping per-thread sharded state instead of one shared mutex:
`cyclelab bandwidth` mode has no lock at all, by construction. Sweeping
its thread count at a fixed 64MB working set: throughput climbed from
16.17 GB/s (1 thread) to 92.76 GB/s (10 threads), scaling far past where
`lock-contention` flattened — genuine improvement — but with visibly
diminishing returns from 6 to 10 threads (75.44 to 92.76 GB/s, a 23%
gain for a 67% increase in threads), the same bandwidth-saturation
signature Chapter 19 already characterized in full. The bottleneck
moved; it did not disappear. Declaring victory at Step 5 alone would be
exactly M17's mistake — the original hotspot (lock contention) shrank,
but the workload's actual scaling ceiling is still there, just relocated
to shared memory bandwidth.

**Steps 6-7 — thread and memory placement.** This is precisely where
this book's own reference machine runs out of hardware to confirm
anything further with: Chapter 24 already established it has no NUMA
topology at all. On genuine multi-socket hardware, the next real
question is whether Step 5's bandwidth ceiling is a true aggregate
-channel limit or a same-socket-crossing problem (`numactl --hardware`,
`numastat -p <pid>`, Chapter 24's Tool View), and if cross-socket traffic
is found, whether it traces back to a first-touch mismatch (Chapter
25) — state initialized on one socket, used from all of them. Both
steps are documented, schematic continuations of this chapter's real
data, following exactly Chapters 24-25's own established honesty about
what this hardware cannot show directly.

**Step 8 — final benchmark and documented limits.** The same Step 2
discipline, applied to whatever the final configuration turns out to
be, is what actually closes an investigation — not a single celebratory
number. On this reference machine, the fully real, measured limit
reached is Step 5's memory-bandwidth ceiling; Steps 6-7's socket-level
limit is a plausible, well-reasoned continuation this specific hardware
cannot confirm, and the final report says exactly that, rather than
either overclaiming a fix that was never actually measured or silently
dropping the question.

## Final report template

- workload and success metric;
- environment;
- baseline distribution;
- initial observations;
- hypotheses considered;
- measurements and overhead;
- changes made;
- before/after evidence;
- bottleneck movement;
- limitations;
- rollback or safety considerations;
- next experiment.

Every field in this template maps directly onto a step in this
chapter's own eight-step Worked Example: "baseline distribution" is Step
2's corrected, repeated measurement; "measurements and overhead" is
Steps 3-4's profiling and tracing, including their own cost (Chapters
10, 26-27's overhead cautions); "bottleneck movement" is exactly Step
5's lock-to-bandwidth transition; "limitations" is this chapter's own
honest Step 6-7 gap on hardware without NUMA.

## Tool View

This chapter deliberately has no single new tool of its own — its
"tool" is the sequencing discipline applied to every tool this book
already built: `cyclelab`'s workload modes (Chapters 1-25), the
sample-based flame-graph pipeline (Chapters 11-15, 26, 29), `getrusage`
context-switch reporting (Chapters 21-22, 29), and the documented,
schematic Linux commands for `perf`, `numactl`, `bpftrace`, and BCC
tooling wherever this reference machine cannot run them directly.

## Guided Lab

**Portability:** **portable** for Steps 1-5 and 8 (real, tested,
reproducible on this reference machine, built entirely from `cyclelab`
and this book's existing tooling); **bare-metal recommended** /
hardware-dependent for Steps 6-7, consistent with Chapters 24-25.

**Setup:** a working `cyclelab` build (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch30_investigation_case_study.sh
```

**Expected qualitative result:** the same eight-step narrative as this
chapter's Worked Example, generated fresh each run — exact numbers vary
run to run (natural scheduling and thermal variance, Chapter 4), but the
qualitative shape holds: `lock-contention` throughput flat across thread
counts, `bandwidth` throughput scaling well past that point before its
own diminishing-returns ceiling appears.

**Interpretation:** running this script end to end *is* a complete,
if compressed, instance of Chapter 5's investigation loop, executed
through every layer this book built one at a time — the point of
Chapter 30 is that none of Chapters 1-29 were independent topics; they
are one investigation's successive layers, and this script is that
investigation, runnable.

**Fallback path:** Steps 6-7's schematic Linux commands are given
directly in the script's own output and in this chapter's Worked
Example above.

**Cleanup:** none.

## Common Misconceptions

This chapter revisits, rather than introduces, several of this book's
own misconceptions, because a real investigation is exactly where each
one would otherwise cause a wrong conclusion:

- **M01/M02** (Chapter 1): this case study's "acceptable average CPU
  utilization" symptom is the opening trap — average utilization staying
  low or moderate says nothing about whether specific threads are
  serialized behind a lock, exactly Steps 3-4's discovery.
- **M15** (Chapter 4): Step 1's single-run benchmark is precisely the
  "one run is an anecdote" failure mode, corrected by Step 2's repeated,
  interleaved measurement.
- **M17** (Chapter 15): Step 5's temptation to declare victory once lock
  contention shrinks, without checking whether the workload outcome
  itself (throughput, this time) actually kept improving as far as it
  should have.
- **M19** (Chapters 22, 29): this case study never leans on
  context-switch counts alone to diagnose the lock-contention
  mechanism — Chapter 29's own captured mutex-wait stacks are what
  actually proves it.

## Practical Implications

Treat every one of this chapter's eight steps as a gate, not a
checklist item to rush through: correcting the benchmark before
profiling, profiling before tracing the specific mechanism, and fixing
one confirmed bottleneck before assuming the next one doesn't exist are
each, individually, exactly where a faster but less disciplined
investigation goes wrong. When hardware limits how far a real
investigation can go — this chapter's own Steps 6-7 — say so explicitly
in the final report, with a documented, reasoned continuation, rather
than either silently stopping or claiming a confirmation that was never
actually measured.

## Key Takeaway

**Good performance engineering is a chain of bounded claims, each
supported by a measurement that was chosen to distinguish competing
explanations.**

## What to Remember

- A multithreaded service's symptoms (acceptable average utilization,
  tail latency, weak scaling, occasional stalls) each point somewhere
  different in isolation; the investigation is the disciplined order in
  which they're resolved, not any single symptom read alone.
- An "invalid" initial benchmark is often not a wrong number — it's a
  benchmark whose design never asked the question that actually
  mattered (Step 1's single, low-concurrency run).
- Fixing one confirmed bottleneck commonly reveals the next one
  underneath (lock contention to memory bandwidth, in this chapter's own
  measured example) — this is success, not a failure to have fixed it
  "completely."
- On hardware that cannot confirm every layer (this reference machine
  has no NUMA topology), the honest final report documents a reasoned,
  schematic continuation rather than overclaiming or silently dropping
  the question.
- The final report template's fields map directly onto this book's own
  investigation loop (Chapter 5) and this chapter's eight-step case
  study — they are the same discipline, formalized for write-up.

## Further Reading

- Brendan Gregg, *Systems Performance: Enterprise and the Cloud*, 2nd
  ed. (Addison-Wesley, 2020) — the single most directly relevant source
  for this chapter's whole-investigation synthesis, cited from Chapter 1
  onward.

## The Next Obvious Question

This book's narrative graph ends here. The next questions are the
reader's own: which of this book's 30 chapters' worth of tools does
*your* next investigation actually need first?
