# The eBPF Mental Model

**Part:** Part VI — Seeing the Invisible
**Concept level:** 6
**Prerequisites:** event, tracepoint, kprobe, kretprobe, uprobe, USDT, argument capture, event rate (Chapter 26)
**New concepts:** eBPF program, hook, verifier, helper, map, per-CPU map, ring buffer, user-space loader, BTF, CO-RE

## Opening Question

How can eBPF safely run custom measurements inside the kernel?

## Incident or Real-World Story

A team wants exactly what Chapter 26 made possible in principle: a
per-call duration histogram for one specific kernel function, running
continuously in production, not just during a debugging session. The
naive version of "run custom code inside the kernel every time this
function is called" sounds, correctly, like a serious risk — a bug in
that custom code could crash the kernel, read memory it has no business
reading, or simply never return, hanging the whole machine. And yet
production systems run exactly this kind of custom, per-event kernel
instrumentation continuously, at scale, without kernel developers
personally auditing every script before it runs. The apparent
contradiction — arbitrary custom logic, running inside the kernel, on a
hot path, safely — is resolved by a specific execution model, not by
trusting the script's author.

## Predict Before Measuring

Before reading further: if you were designing a system to let
un-trusted, arbitrary code run inside a kernel safely, what would you
refuse to allow that code to do? Try to name at least three concrete
restrictions before continuing.

## Worked Example

Contrast two ways of building the same duration histogram from Chapter
26's function-entry/return probes. The naive approach: on every function
return, copy the computed duration out to user space (a context switch
and a data copy, every single call), and let a user-space program
maintain the histogram. The eBPF approach: the in-kernel program itself
updates a histogram stored in a **map** — a kernel-resident data
structure the program and user space can both access — incrementing one
bucket counter per call, entirely inside the kernel, and only the
finished, aggregated histogram is ever read out to user space, once,
whenever the operator asks for it. The naive approach pays a user-space
round trip on every event; the eBPF approach pays it once for the whole
measurement window, no matter how many events occurred.

## Core Intuition

An **eBPF program** is a small, restricted piece of code, written by a
user, that the kernel agrees to run at a specific **hook** — an
attachment point that can be one of Chapter 26's probes (a tracepoint,
kprobe, kretprobe, or uprobe) or a number of other kernel subsystems
entirely (networking, security policy) outside this book's scope. Before
any of that code runs even once, the **verifier** statically analyzes it
and refuses to load anything it cannot prove is safe: bounded loops
only, no arbitrary pointer arithmetic, no access outside memory the
program can prove it owns, and a guaranteed termination. Because the
verifier deliberately cannot express everything a general-purpose
language can, eBPF programs call **helpers** — a fixed, kernel-provided
set of functions for the specific things a verified, sandboxed program
is still allowed to do (read a value safely, get the current time,
write to a map). **Maps** are the kernel-resident data structures
(hash tables, arrays, and more specialized shapes) that let a program
accumulate state across many separate invocations and let user space
read the results; a **per-CPU map** keeps a separate copy per CPU
specifically to avoid the same cross-core coherence cost Chapter 18
already made concrete, since a single shared counter updated from every
CPU would itself become a bottleneck. A **ring buffer** is a
purpose-built map variant for streaming individual events out to user
space efficiently, rather than only aggregating them in place. A
**user-space loader** is the ordinary program (compiled from a tool like
`bpftrace`, BCC, or a hand-written libbpf application) that presents the
eBPF bytecode to the kernel, reads back map contents, and manages the
program's lifecycle. **BTF** (BPF Type Format) is embedded type
information that lets a program describe kernel and user data structures
precisely; **CO-RE** ("Compile Once, Run Everywhere") uses that type
information to let one compiled program adapt itself to small
differences in struct layout across kernel versions, instead of
requiring a fresh compile against each target kernel's exact headers.

A useful mental picture: an eBPF program is a **temporary sensor
attached to a defined point in a running machine** — installed at a
specific, deliberately chosen location, wired to report only the
specific readings it was built to take, and removable without touching
the machine's own operation. The constraint that makes the analogy
worth keeping precise: a sensor is a constrained, purpose-built
instrument, not passive magic that happens to know things — everything
it can report was explicitly programmed in, checked for safety before
installation (the verifier), and limited to the specific attachment
point (the hook) it was built for.

## Core execution model

1. user space defines or loads a program;
2. the kernel verifier checks allowed control flow and memory access;
3. the program attaches to a supported hook;
4. the hook triggers the program;
5. the program aggregates data in maps or emits selected events;
6. user space reads and presents the results.

This is precisely this chapter's Worked Example, generalized: step 5 is
where the naive approach's per-event user-space round trip gets replaced
by an in-kernel aggregation step, and step 6 is where the finished
result finally crosses back into user space — once, not once per event.

## Technical Explanation

The verifier is the mechanism that makes the apparent contradiction in
this chapter's opening story resolvable at all: rather than trusting the
program's author, the kernel proves specific safety properties about the
program itself before it is ever allowed to execute a single instruction
on real hardware. This is why eBPF programs cannot contain unbounded
loops (the verifier must be able to bound the program's own running
time), cannot dereference arbitrary pointers (every memory access must
be provably within a region the program is allowed to read), and cannot
call arbitrary kernel functions (only the fixed, audited set of helpers
is available). This is also precisely why eBPF is a genuinely different
proposition from a kprobe alone: Chapter 26's probes decide *where*
measurement happens; eBPF decides *what safely-bounded logic* is allowed
to run once it fires. BTF and CO-RE solve a separate, practical problem
that has nothing to do with safety: a program compiled against one
kernel's exact internal struct layouts would ordinarily need
recompiling — or at least careful adjustment — for every different
target kernel version. CO-RE uses the running kernel's own BTF
information to patch up field offsets automatically, so one compiled
artifact can, within limits, run correctly against a range of kernel
versions rather than exactly one.

## Tool View

- What is measured: whatever the eBPF program's author chose to
  aggregate in a map or emit through a ring buffer — the mechanism
  itself is general-purpose; what it observes depends entirely on where
  it's hooked (Chapter 26) and what it does once triggered.
- What is not measured: anything the verifier refused to allow the
  program to compute, and anything the hook itself never exposes as an
  argument — eBPF cannot see more than the hook and the helpers make
  available to it.
- Required permissions: loading eBPF programs has historically required
  root or `CAP_SYS_ADMIN`; more recent kernels can gate specific,
  narrower operations behind `CAP_BPF` and `CAP_PERFMON` instead, but in
  practice, most systems and most of Chapter 28's tooling still expect
  privileged access.
- Likely overhead: the verifier's safety guarantees do not mean zero
  cost — every helper call, every map update, and every event emitted
  through a ring buffer has a real, if small, cost, multiplied by
  whatever the underlying hook's event rate (Chapter 26) turns out to
  be.
- Portability: **Linux-only**, and further constrained by kernel
  version — older kernels lack newer helpers, map types, and BTF/CO-RE
  support entirely. `make doctor`'s "BTF" section checks the one
  concrete, real prerequisite this reference machine can still verify
  meaningfully even without running eBPF at all: whether
  `/sys/kernel/btf/vmlinux` exists on a given Linux target, which is
  exactly what CO-RE needs to work.
- Common failure mode: a program that the verifier rejects, often for a
  reason that is not obvious from the program's logic alone (an
  insufficiently bounded loop, a pointer access the verifier cannot
  prove is safe even though it happens to be) — verifier rejections are
  a normal, expected part of eBPF development, not a sign that something
  is broken.

## Guided Lab

**Portability:** **privileged** and **Linux-only** — eBPF has no
meaning on this book's own macOS reference machine at all, unlike
Chapter 26's tracing chapter, which at least had dtrace as a
structurally similar (if practically unusable here) analog. There is no
portable substitute for actually loading and running a verified eBPF
program.

**Setup:** none.

**Exercise — predict the verifier's decision.** For each of the
following four hypothetical eBPF program sketches, decide whether you
expect the verifier to accept or reject it, and why, *before* reading
the answer that follows it:

1. A loop that walks a linked list until it finds a NULL pointer.
2. A loop with a fixed, compile-time-constant trip count of 10.
3. A helper call that reads a fixed number of bytes from a
   user-supplied pointer, after first checking that pointer against a
   known-valid range.
4. A helper call that reads a fixed number of bytes from a
   user-supplied pointer, with no check beforehand.

**Expected qualitative result (do not read until you've predicted all
four):**

1. **Rejected.** A linked list's length is not something the verifier
   can bound at load time — this is exactly the "no unbounded loops"
   restriction from this chapter's Technical Explanation, regardless of
   whether the list happens to be short in practice.
2. **Accepted.** A fixed, compile-time trip count is exactly the kind of
   loop the verifier can prove terminates, and modern verifiers
   additionally support bounded loops with a provable, if
   runtime-determined, upper limit.
3. **Accepted.** This is precisely the "provably within a region the
   program is allowed to read" pattern this chapter's Technical
   Explanation describes — the check makes the subsequent access
   provably safe.
4. **Rejected.** An unchecked pointer read is exactly what the verifier
   exists to prevent — it cannot prove the access is safe, so it refuses
   to load the program at all, rather than allowing it to run and
   potentially fault or read memory it shouldn't.

**Interpretation:** none of this requires running real eBPF bytecode to
be a genuine, testable exercise in this chapter's actual mental model —
the verifier's accept/reject boundary is a specific, learnable set of
rules, not a black box, and predicting it correctly is direct evidence
you've internalized Chapter 27's core mechanism rather than just its
vocabulary.

**Fallback path:** on genuine Linux hardware with root access, `bpftrace
-e 'kprobe:do_sys_openat2 { printf("%s\n", comm); }'` (previewing
Chapter 28's syntax) followed by intentionally breaking it — e.g.
replacing the fixed `printf` with an unbounded loop — makes the
verifier's rejection message a real, tested confirmation of exercise
item 1 above; **documented, not tested, on this reference machine.**

**Cleanup:** none.

## Common Misconceptions

### *"eBPF is a background daemon." (M38)*

**Why it's wrong:** It inverts where the logic actually runs: an eBPF
program is loaded into the kernel and executes synchronously, in the
context of whatever triggered its hook — there is no separate always-on
service process doing the measuring. User-space tools like `bpftrace`
and BCC are loaders and readers, not the thing collecting the data.

**Correct intuition:** The kernel itself runs the verified program,
directly at the hook, every time it fires; user space's job is to load
it once and read the results.

**Analogy:** A smoke detector doesn't run on a schedule from some
central office — it's wired directly into the room and fires the
instant it senses smoke. There's no daemon patrolling the building; the
sensor itself is triggered by the event.

### *"eBPF can safely execute arbitrary kernel code." (M39)*

**Why it's wrong:** This overstates what eBPF permits, not what it
requires. The verifier specifically restricts eBPF programs to a
provably bounded, provably memory-safe subset of what's possible; it
cannot and does not run arbitrary kernel code, and a program that
attempts to violate the verifier's constraints is rejected before it
runs at all, not sandboxed at runtime.

**Correct intuition:** Safety comes from refusing to load anything
unproven, not from containing something dangerous after the fact.

**Analogy:** A building inspector doesn't let a contractor build
whatever they want and then demolish anything unsafe afterward — the
blueprint gets rejected before a single nail is hammered if it doesn't
meet code.

### *"CO-RE makes every program portable to every kernel." (M40)*

**Why it's wrong:** CO-RE adapts a compiled program to differences in
struct *layout* using BTF information; it cannot invent a helper, map
type, or hook that an older target kernel simply does not have at all.

**Correct intuition:** A CO-RE program built assuming a recent kernel
feature will still fail to load on a kernel that predates that feature,
portability adjustments notwithstanding.

**Analogy:** A universal power adapter lets your laptop's plug fit a
foreign outlet's shape, but it can't produce electricity in a building
that has no power at all — it adapts the connector, not the underlying
capability.

### *"Maps are ordinary user-space hash maps." (M41)*

**Why it's wrong:** A map is a kernel-resident data structure, defined
and sized at load time, accessed through a narrow, verified API from
inside the eBPF program and through a separate system-call interface
from user space — it is not the same thing as an in-process hash table
a user-space program could resize or iterate however it likes.

**Correct intuition:** Per-CPU maps in particular exist specifically to
avoid a cost (cross-core coherence traffic, Chapter 18) that an
ordinary shared hash map would incur under concurrent update.

**Analogy:** A shared filing cabinet bolted to the office wall, with a
strict sign-out procedure for every drawer, behaves very differently
from a notebook you keep at your own desk and scribble in freely — one
has rules and structure built in from the start, the other doesn't.

### *"eBPF is always zero-overhead." (M13)*

**Why it's wrong:** Every helper call, map update, and emitted event
has a real cost, and that cost is multiplied by whatever the hook's
actual event rate turns out to be — the same overhead equation Chapter
26 already established for kprobes applies here too, because eBPF sits
on top of exactly those hooks, not somewhere the cost disappears.

**Correct intuition:** Budget for overhead as rate times per-event work,
the same equation as any other probe, before deploying broadly.

**Analogy:** A security camera that's "just recording" still uses
power, storage, and bandwidth for every frame — being unobtrusive isn't
the same as being free, and a camera on a busy street costs more to run
than one pointed at an empty hallway.

## Practical Implications

Expect verifier rejections as a normal part of writing or adapting eBPF
programs, not a sign of a broken toolchain — the rejection reason
usually points directly at an unprovable loop bound or an unchecked
memory access. Budget for per-event overhead using the same event-rate
reasoning Chapter 26 introduced, since eBPF's safety guarantees are
about correctness and isolation, not cost. When choosing between a map
that aggregates in place and a ring buffer that streams individual
events, prefer aggregation whenever the question is about a distribution
or a total, and reserve event streaming for cases where the individual
events themselves, not just their aggregate, are what's needed — the
same in-kernel-aggregation-over-per-event-transfer logic this chapter's
Worked Example already demonstrated.

## Key Takeaway

**eBPF is a constrained in-kernel execution model that turns selected
events into bounded measurements and aggregates.**

## What to Remember

- An eBPF program only runs at all if the verifier can prove it
  terminates and only accesses memory it can prove is safe — this is
  the mechanism that resolves this chapter's opening safety concern, not
  trust in the program's author.
- Hooks are Chapter 26's tracepoints, kprobes, kretprobes, and uprobes
  (plus other kernel subsystems outside this book's scope); eBPF decides
  what bounded logic runs once a hook fires, not where it fires.
- Maps let a program aggregate state across many invocations entirely
  inside the kernel, so only the finished result crosses into user
  space — the core efficiency advantage over per-event user-space round
  trips.
- Per-CPU maps avoid cross-core coherence cost under concurrent update,
  the same false-sharing-adjacent concern Chapter 18 introduced.
- BTF and CO-RE solve kernel-struct-layout portability, not safety or
  feature availability — a CO-RE program still needs the target kernel
  to actually have whatever helpers and hooks it depends on.
- eBPF has no meaning outside Linux; this book's macOS reference machine
  cannot run it at all, unlike Chapter 26's dtrace, which is at least
  structurally comparable.

## Further Reading

- Linux BPF documentation: <https://docs.kernel.org/bpf/>
- libbpf overview: <https://docs.kernel.org/bpf/libbpf/libbpf_overview.html>
- BPF CO-RE reference guide: <https://nakryiko.com/posts/bpf-core-reference-guide/>

## The Next Obvious Question

How do `bpftrace` and BCC turn questions into live instrumentation?
