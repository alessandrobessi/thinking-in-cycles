# Off-CPU Time, I/O Latency, and Contention

**Part:** Part VI — Seeing the Invisible
**Concept level:** 7
**Prerequisites:** probe specification, aggregation, histogram, stack aggregation (Chapter 28); context switch, blocked, sleeping (Chapter 21-22)
**New concepts:** off-CPU time, blocked stack, wake-up latency, sleep, futex wait, lock contention, block I/O, queueing delay, off-CPU flame graph

## Opening Question

Where does time go when a thread is not on a CPU?

## Incident or Real-World Story

Chapter 21 already established that a thread only consumes CPU while
running, and that its latency can still grow while it waits runnable in
a queue. This chapter's story is the natural extension: a request's
total latency is the sum of every state a thread passes through, on-CPU
or not, and every profiling tool built so far in this book — `perf
record`/`sample` (Chapter 12), flame graphs (Chapter 14), even Chapter
26-28's event tracing when hooked to on-CPU-only events — shares one
specific blind spot. None of them, by default, sees time a thread spends
not running at all: blocked on a lock, waiting for a wake-up after a
scheduling decision, waiting on a disk or network read to complete, or
simply asleep on purpose. A service can show a beautifully clean on-CPU
flame graph — every sampled stack inside reasonable, well-optimized
code — and still have terrible p99 latency, because the missing time
never showed up in a profile that only looks while the thread is
running.

## Predict Before Measuring

Before reading further: if a request's total latency is 50ms, and an
on-CPU profile shows only 8ms of that request's thread actually
executing instructions, where do you expect the other 42ms went, and
would an on-CPU flame graph — however carefully read — ever be able to
show you?

## Worked Examples

- a thread blocks on a mutex, waiting for another thread to release it;
- a request's thread is woken up (made runnable) but waits in a run
  queue behind other work before actually resuming (Chapter 21's own
  mechanism, revisited from the waiting side);
- a synchronous read waits on storage to return data;
- a thread sleeps intentionally, deliberately yielding the CPU rather
  than being blocked by anything external;
- an asynchronous runtime parks one logical task so another can run on
  the same underlying thread, a cooperative, intentional version of the
  same not-running state.

## Core Intuition

Chapters 1 and 21 already used **on-CPU time** and **off-CPU time**
informally, to name the two states a thread's time divides into without
yet formalizing either; this chapter completes that: **on-CPU time** is
any interval a thread spends actually executing instructions on a CPU,
and **off-CPU time** is any interval during which a thread exists but is
not executing instructions on a CPU — the necessary complement to
everything Chapters 6-20 built about what happens *while* a thread runs.
A **blocked stack** is the call stack captured at the moment a thread
stopped running, showing *where in the code* it stopped, which is what
turns "this thread was off-CPU for 40ms" into "this thread was off-CPU
for 40ms waiting inside this specific mutex acquire call." **Wake-up
latency** is the gap between a thread becoming logically able to run
again (its dependency resolved, its I/O complete) and actually resuming
execution — Chapter 21's run-queue waiting, named from the perspective
of the thread doing the waiting rather than the scheduler doing the
placing. **Sleep** is a thread intentionally yielding the CPU for a
bounded time, an off-CPU state with no external blocker at all. A
**futex wait** is the specific Linux mechanism most user-space lock
implementations (including `pthread_mutex`) use to put a thread to sleep
efficiently while it waits for a contended lock, rather than spinning
and burning CPU while waiting. **Lock contention** is what Chapter 29's
own guided lab measures directly: multiple threads wanting the same
lock at overlapping times, at least one of them necessarily spending
time off-CPU as a direct result. **Block I/O** is a request to a block
storage device (a disk read, for instance); the issuing thread is
typically off-CPU for the entire round trip. **Queueing delay** is time
spent waiting for a resource — a lock, a run queue slot, an I/O
device — that is busy or contended, as distinct from the time actually
using that resource once it becomes available. An **off-CPU flame
graph** applies flame graph's population-of-stacks visualization
(Chapter 14) to blocked stacks and their off-CPU durations instead of
on-CPU sampled time, turning "which code paths spend the most total time
not running" into the same readable shape Chapter 14 already taught you
to interpret.

## Technical Explanation

The mechanism that makes off-CPU accounting possible at all is
symmetrical to on-CPU sampling, not an extension of it: instead of
periodically asking "what is running right now" (Chapter 11's sampling
model), an off-CPU-aware tool records a timestamp and a stack every time
a thread *stops* running (a context switch out, Chapter 22) and again
every time it *resumes*, computing the difference as that thread's
off-CPU duration for that specific interval, attributed to the stack it
was blocked in. This is exactly why dedicated off-CPU tools generally
build on Chapter 26-28's tracing machinery rather than a periodic
sampler: sampling can only characterize on-CPU time statistically, but
off-CPU accounting needs to know precisely when each transition
happened, which requires an event fired at the transition itself, not a
statistical guess at a fixed interval.

This book's own reference machine offers a genuinely useful partial
substitute, though, and it's worth being precise about exactly how it
differs from a dedicated off-CPU tool. macOS's `sample`(1) (used
throughout Chapters 11-15 and 26) records every live thread's stack on a
wall-clock interval, **regardless of whether that thread is currently
running or blocked** — unlike `perf record`'s on-CPU-only default. This
means a `sample` capture of a heavily lock-contended workload genuinely
does show blocked threads' stacks, sitting inside the mutex-wait code
path, for a real fraction of the total captured samples. What it does
*not* do is what a dedicated off-CPU tool does: `sample` cannot tell you
that a specific interval was off-CPU rather than on-CPU just from one
sample — it needs the frame's identity (recognizably a wait/blocking
function) to tell the difference after the fact, and it cannot measure
exact wake-up latency the way a paired context-switch-out/switch-in
trace could. This chapter's Guided Lab uses it anyway, honestly, for
exactly what it can show: a genuine, measured shift in where sampled
time landed when a workload goes from purely on-CPU to heavily
lock-contended.

## Tool View

- What is measured: which code path a thread was executing (on-CPU
  tools) or blocked inside (off-CPU tools), and for how long, at the
  granularity the specific tool captures transitions or samples state.
- What is not measured: an on-CPU-only tool cannot see off-CPU time at
  all (Chapter 12's M08); conversely, most off-CPU tools do not also
  give you the on-CPU profile in the same pass, so both are usually
  needed together for a complete picture — this chapter's own Guided
  Lab captures both sides specifically so they can be compared.
- Required permissions: Linux off-CPU tools generally build on Chapter
  26-28's tracing infrastructure and inherit the same privilege
  requirements; block I/O latency histograms similarly.
- Likely overhead: proportional to how often threads transition
  off-CPU and back — a workload with very frequent, very short blocking
  periods can make an event-based off-CPU tool considerably more
  expensive than the same tool applied to a workload with fewer, longer
  blocking periods.
- Portability: dedicated off-CPU tools (`offcputime-bpfcc` and similar)
  are Linux-only, per Chapters 26-28. macOS's `sample`(1), used
  throughout this Part, gives a genuine but limited substitute, exactly
  as described above.
- Common failure mode: reading an on-CPU-only flame graph as if it
  already accounts for all latency, and concluding a workload has "no
  bottleneck" because the visible, on-CPU code all looks reasonable —
  precisely this chapter's opening story.

## Guided Lab

**Portability:** **portable** for the `sample`-based on-CPU-vs-blocked
comparison (builds directly on Chapters 11-15 and 26's existing
toolchain, real and tested on this reference machine); **privileged**
and **Linux-only, documented but not tested** for the dedicated
run-queue-latency and block-I/O-latency BCC tools below.

**Setup:** a working `cyclelab` build (`make lab-cyclelab`).

**Command:**

```bash
./labs/scripts/ch29_offcpu_lock_contention.sh
```

**Expected qualitative result:** on this book's reference machine, a
`compute`-mode capture (pure on-CPU busy work, no blocking by
construction) showed **0.0%** of sampled stack frames inside a
mutex-wait code path; an equivalent-length `lock-contention`-mode
capture, with 8 threads contending one mutex at `--hold-us=300`, showed
**77.8%** of sampled frames inside `__psynch_mutexwait`/
`_pthread_mutex_firstfit_lock_wait` — a real, measured, dramatic shift
from a workload that is architecturally incapable of blocking to one
whose entire design is built around threads waiting on each other. The
script also renders `contention.svg`, a real flame graph in which frames
recognizably inside the mutex-wait path represent genuinely captured
off-CPU time, not inferred or simulated.

**Interpretation:** this is exactly this chapter's Core Intuition made
concrete — the same sampling mechanism, pointed at a workload that
blocks instead of one that doesn't, surfaces the blocked stacks
directly, because `sample` (unlike `perf record`'s default) does not
filter out non-running threads. The gap between 0.0% and 77.8% is the
off-CPU fraction Chapter 12's on-CPU-only tools would have reported as
simply "not captured" — invisible, not zero.

**Fallback path and Linux equivalents (documented, not tested on this
reference machine):**

```bash
# Run-queue latency: a packaged BCC tool (Chapter 28's Progression step 7)
sudo runqlat 5 1

# Off-CPU profile for lock contention, collected as its own flame graph
sudo offcputime-bpfcc -f 30 > offcpu.folded
# render with the same folded-stack -> SVG pipeline this book already uses:
python3 labs/scripts/flamegraph_svg.py offcpu.folded -o offcpu.svg

# Block I/O latency histogram
sudo biolatency 5 1

# Compare on-CPU and off-CPU flame graphs for the same workload:
sudo profile -F 99 -f 30 > oncpu.folded          # on-CPU (Chapter 12-style)
sudo offcputime-bpfcc -f 30 > offcpu.folded       # off-CPU (this chapter)
```

**Cleanup:** remove the script's output directory (default `/tmp/ch29_offcpu`) if desired.

## Common Misconceptions

### *"Off-CPU time is automatically waste." (M42)*

**Why it's wrong:** This is the mandatory caution for this entire
chapter, elevated to a full misconception because it's exactly the
wrong lesson to take from everything above: a thread waiting is
frequently correct, intentional, or imposed by a dependency entirely
outside the program's control. A thread sleeping on purpose (this
chapter's fourth Worked Example) is off-CPU by design, not by failure;
a thread waiting on a genuinely necessary I/O round trip is bounded by
the storage or network, not by anything the profiled code did wrong.

**Correct intuition:** Off-CPU time is *information* — where the time
went, and why — not an automatic verdict. Compare the off-CPU duration
against what the operation being waited on could plausibly take at
minimum (a network round trip has a physical floor; a lock held for
exactly as long as necessary has a floor set by the critical section's
own real work) — time far beyond that floor is where contention, not
necessity, is the likely explanation.

**Analogy:** A doctor's patient sitting in the waiting room isn't
automatically evidence something's wrong — some of that time is a
necessary wait for a scheduled appointment slot, and only a wait far
longer than any appointment should take signals an actual problem.

### *"Sampling profiles show all latency." (M08, fully resolved here)*

**Why it's wrong:** Chapters 12 and 26 each surfaced a piece of this;
this chapter completes it: a profiler that only samples on-CPU state
can, by construction, never show off-CPU time at all, no matter how
long it runs or how carefully its output is read.

**Correct intuition:** The two 0.0%/77.8% numbers in this chapter's own
lab exist only because the sampler used here happens to also capture
blocked threads, which is a property of the specific tool (`sample`),
not something true of sampling profilers in general.

**Analogy:** A fitness tracker that only counts steps will report a
"perfectly idle" day for someone who spent eight hours lifting weights
without moving their feet — the tool's blind spot, not the person's
actual activity, is what's missing from the report.

### *"Context-switch counts alone diagnose scheduler overhead." (M19, revisited)*

**Why it's wrong:** This chapter's own lab surfaces a sharper version
of the same caution from a different direction: on this reference
machine, `ru_nivcsw` climbed from roughly 600 (compute, no blocking) to
roughly 5,800-6,200 (lock-contention, heavy blocking) — a real, large
jump — but `ru_nvcsw` stayed at exactly 0 in both cases, a confirmed
limitation of this machine's `getrusage` implementation (documented in
`labs/cyclelab/README.md`), not evidence that lock-contention wasn't
genuinely causing threads to block.

**Correct intuition:** The count alone, especially on this platform,
cannot distinguish "blocked on a lock" from "preempted while still
runnable" — the mutex-wait stacks captured directly in this chapter's
flame graph are what actually proves the blocking, not the
context-switch counters.

**Analogy:** A doorbell that rang twenty times today tells you the door
was answered twenty times, but not whether those were twenty deliveries
or the same person leaning on the button — the count needs the
security footage to actually mean something.

## Practical Implications

Pair an on-CPU profile with an off-CPU view before declaring a workload
free of bottlenecks — a clean on-CPU flame graph proves the visible code
is efficient while it runs, and proves nothing about time spent not
running at all. When off-CPU time shows up, compare it against a
plausible minimum for whatever it's waiting on before treating it as a
problem; genuinely necessary waiting (an intentional sleep, an
unavoidable I/O round trip) looks identical to wasteful waiting in a
raw off-CPU duration alone, and only the blocked stack's identity and a
sense of the operation's real minimum cost can tell them apart.

## Key Takeaway

**A complete latency explanation joins on-CPU execution with the
reasons, locations, and durations for which critical threads were not
running.**

## What to Remember

- Off-CPU time is any interval a thread is not executing instructions;
  on-CPU-only tools cannot see it by construction, not by a fixable
  limitation of how carefully you read their output.
- A blocked stack identifies where a thread was waiting; wake-up latency
  is specifically the gap between becoming runnable again and actually
  resuming.
- Futex waits are how most user-space locks (including `pthread_mutex`)
  put a thread to sleep efficiently while contended, rather than
  spinning.
- macOS's `sample`(1) captures blocked threads too (wall-clock interval,
  all thread states), unlike `perf record`'s on-CPU-only default — a
  real, tested, if imperfect, substitute for a dedicated off-CPU tool on
  this reference machine.
- Off-CPU time is not automatically waste (M42) — compare its duration
  against a plausible minimum for whatever it's waiting on before
  treating it as a problem.
- On this reference machine, `ru_nvcsw` (voluntary context switches)
  reads 0 in every workload tested, including genuinely blocking ones —
  a confirmed platform limitation; a mutex-wait stack in a real profile
  is stronger evidence of blocking than this counter is, here.

## Further Reading

- Brendan Gregg, "Off-CPU Analysis," <https://www.brendangregg.com/offcpuanalysis.html>
- Linux BPF documentation: <https://docs.kernel.org/bpf/>

## The Next Obvious Question

How do all the layers combine into one defensible investigation?
