# Stacks, Symbols, and Unwinding

**Part:** Part III — Where the CPU Time Goes
**Concept level:** 3
**Prerequisites:** sample, call graph, annotation, source mapping (Chapter 12)
**New concepts:** symbol table, debug information, stripped binary, stack unwinding, frame pointers, DWARF, last branch records, JIT symbols, kernel symbols

## Opening Question

Why are call stacks sometimes missing or wrong?

## Incident or Real-World Story

A team profiles a production service and gets back a call graph full of
raw hexadecimal addresses instead of function names — `0x7f3a2c001120`
instead of `handle_request`. Nobody touched the code differently between
the working profile from last month and this one; what changed was the
deploy pipeline, which switched to shipping binaries with debug symbols
stripped out to save a few megabytes of image size. The profiler was
working exactly as designed. It had simply lost access to the one thing
that lets it translate "the CPU was executing address X" into "the CPU
was executing function Y" — and without that translation, a profile is
just a list of numbers.

A second, quieter version of the same problem shows up later: a
different service's profiles have function names, but the call stacks
routinely stop two or three frames short of anything meaningful —
`handle_request` appears, but nothing above it, no hint of which
endpoint or caller triggered it. That team hadn't stripped symbols. They
had, for an unrelated reason, built with an option that omits frame
pointers, and the profiler's stack-walking logic — on that particular
platform, with that particular unwinding method — had nothing reliable
left to follow past the first frame or two.

## Predict Before Measuring

Before reading further: two builds of the exact same source, run under
the exact same profiler — one with full debug information, one without.
Which do you predict will still show correct *function names*: only the
first, both, or neither? Which will show correct *source line numbers*
within a function: only the first, both, or neither? These are two
different questions with two different answers — hold both predictions
for the Guided Lab.

## Worked Example

Four real ways a profiler's call stack can come back missing or wrong,
two of which this chapter's lab reproduces directly and two worth
reasoning through:

- **A stripped binary with unresolved addresses.** This chapter's
  opening story: without a **symbol table** mapping addresses back to
  function names, a profiler can only report raw addresses.
- **A frame-pointer-omitted build with broken call chains.** This
  chapter's lab reproduces this directly: without frame pointers (or
  another reliable unwinding mechanism), a profiler's **stack
  unwinding** — the process of reconstructing the chain of callers from
  a single snapshot — can run out of reliable information partway up
  the stack, even when the innermost frame is correctly identified.
- **A JIT runtime requiring its own symbol integration.** Code generated
  at runtime (by a JIT compiler) has no fixed address-to-symbol mapping
  a profiler can read from a static binary at all; without the runtime
  cooperating to expose **JIT symbols** as they're generated, that code
  shows up as unresolved no matter how good the profiler is.
- **Containers whose symbols aren't available on the host.** A profiler
  running on a container host, observing a process inside a different
  filesystem namespace, may simply not have access to the binary (or its
  separate debug-info file) the guest process is actually running,
  independent of whether that binary was built with symbols at all.

## Core Intuition

A **symbol table** maps compiled addresses back to names (function and,
depending on detail, variable names) — it's what turns `0x7f3a2c001120`
into `handle_request`, and it can survive independently of source-line
detail. **Debug information** (commonly in **DWARF** format on Linux and
macOS) goes further: it maps addresses back to *source file and line*,
enabling the annotation Chapter 12 relied on — and it's a separate piece
of data from the symbol table, which is exactly why a binary can have
one without the other. A **stripped binary** has had some or all of this
metadata deliberately removed, usually to reduce file size or avoid
shipping source-level detail. **Stack unwinding** is how a profiler
reconstructs the *chain* of callers from a single sampled snapshot —
commonly by following saved **frame pointers** (a convention where each
function records where its caller's frame is) or, on platforms and
architectures that support it, less invasively via
compiler-generated unwind tables. **Last branch records** (a hardware
feature on some CPUs recording recent branch history) are an
alternative unwinding source on architectures that support them,
independent of frame pointers entirely. **Kernel symbols** are a
parallel concern one layer down: resolving addresses inside kernel code
a profiled program transitioned into, subject to its own, separate
permission and availability rules (`kptr_restrict`, from Chapter 10).

## Technical Explanation

This chapter's lab isolates two genuinely independent variables that are
easy to conflate: whether debug information is present (governs source
*line* resolution) and whether frame pointers are present (governs how
completely the *call chain* can be reconstructed). A build can have
either without the other, and a real, tested comparison shows both
effects cleanly: dropping debug information (`-g` omitted) leaves
function *names* intact — because those come from the symbol table,
built regardless of the `-g` flag — but source-line numbers disappear
entirely. Omitting frame pointers (`-fomit-frame-pointer`) leaves source
lines intact wherever debug information is present, but can leave the
reconstructed call chain stopping short, missing ancestor frames the
unwinder had no reliable way to find.

**Policy worth stating directly: this book does not recommend building
with frame pointers universally.**
Omitting them is a real, sometimes meaningful optimization (freeing a
register, avoiding the overhead of maintaining the chain on every call);
keeping them is what makes cheap, reliable stack unwinding possible for
tools like the ones in this Part. Which one is right depends on whether
profiling this specific binary, on this specific platform, is a
priority worth the tradeoff — not a rule to apply everywhere by default.

## Tool View

- What is measured: this chapter's lab compares three builds of the
  identical source under macOS `sample`: full debug info, no debug info,
  and debug info with frame pointers omitted.
- What is not measured: JIT symbol integration and container-boundary
  symbol availability (two of this chapter's four Worked Example cases)
  aren't reproducible with a static C binary like `cyclelab` — they're
  included for completeness of the mental model, not as tested claims.
- Required permissions: none for this chapter's lab.
- Likely overhead: none beyond normal compilation and sampling.
- Portability: on Linux, the equivalent comparison uses `gcc`/`clang`
  with and without `-g`, and with and without `-fomit-frame-pointer`,
  profiled with `perf record -g` (which itself supports multiple
  unwinding methods — frame-pointer-based, DWARF-based via
  `--call-graph dwarf`, and, on supporting Intel CPUs, last-branch-record
  based via `--call-graph lbr` — a choice this book deliberately doesn't
  pick one "correct" answer for, since the right method depends on what
  the target binary was actually built with).
- Common failure mode: seeing missing or wrong symbols and assuming the
  profiler is broken, rather than checking what metadata the build
  actually shipped.

## Guided Lab

**Portability:** portable.

**Setup:** none beyond a working C compiler; the script builds all three
variants itself. Requires macOS's built-in `sample`(1).

**Command:**

```bash
./labs/scripts/ch13_symbol_availability.sh
```

**Expected qualitative result:** the full-debug-info build should show
both readable source lines and an unbroken main-thread ancestry back
through `main` and `start`. The no-debug-info build should still show
correct function *names* but no source-line numbers. The
frame-pointer-omitted build should show source lines fine, but a
main-thread ancestry that stops short. One real run on the reference
machine for this book showed exactly this pattern — the
frame-pointer-omitted main thread's reconstructed stack stopped at
`compute_run`, never reaching `main` or `start`, while the same thread's
stack reached all the way back in both other builds.

**Interpretation:** the two failures are independent and look different:
missing source lines (no `-g`) versus a truncated ancestor chain
(`-fomit-frame-pointer`) are not the same symptom, and correctly
diagnosing which one you're looking at determines what fix is even
possible — no debug information can't be fixed by asking the profiler to
try harder; it needs a rebuild. Do not expect identical unwinding
behavior on a different OS or architecture: how much a missing frame
pointer breaks unwinding specifically depends on what alternative
unwind information the platform and toolchain provide.

**Fallback path:** if `sample` isn't available, the underlying point can
still be verified without profiling at all: run `nm` on a binary built
with and without `-g` — both list the same function names, confirmed
directly on this book's reference machine, since `nm` reads the symbol
table, not debug info. Then look up one of those symbols' address with
a source-aware tool: on macOS, `atos -o <binary> <address>` resolves to
`compute_worker (in ch13-full) (compute.c:56)` for the `-g` build and
only `compute_worker (in ch13-nodebug) + 0` (no source line) for the
build without it — tested directly for this chapter. On Linux, the
equivalent check is `objdump --dwarf=decodedline <binary>`, which lists
real address-to-source-line rows for a `-g` build and nothing at all
for one without — confirming the symbol-table/debug-information
distinction directly, independent of any profiler.

**Cleanup:** the script's build outputs land in `labs/cyclelab/bin/` as
`ch13-full`, `ch13-nodebug`, and `ch13-nofp`; remove them with `make
clean` from `labs/cyclelab/` (or delete them directly) if you don't want
them lingering alongside the book's regular `cyclelab`/`cyclelab-debug`
binaries.

## Common Misconceptions

There is no blueprint-seeded misconception registry entry specific to
this chapter; two are worth naming directly. **"A profiler that shows
wrong or missing symbols is buggy."** This is wrong because a profiler
can only report what the binary's metadata actually makes available —
missing symbols are almost always a build or deployment property (a
stripped binary, a missing frame pointer, a JIT runtime that didn't
cooperate), not a defect in the profiling tool itself. The evidence that
distinguishes the two: profile the identical workload built two
different ways, as in this chapter's lab — the same profiler produces
visibly different call-stack quality purely from what the build
provided it.

**"Frame pointers should always be kept for profiling, no exceptions."**
This is wrong because keeping frame pointers is a real, sometimes
meaningful runtime cost (a reserved register, per-call bookkeeping), and
alternative unwinding methods (DWARF-based, or hardware last-branch
records where supported) can recover much of the same information
without paying it — whether the tradeoff is worth it depends on the
platform and how much profiling fidelity matters for that specific
binary. The evidence that distinguishes the two: check whether your
platform's profiler supports a non-frame-pointer unwinding method before
assuming frame pointers are the only path to a usable call stack.

## Practical Implications

Before concluding a profiler is broken or a workload is unprofilable,
check what the binary actually shipped: symbols, debug information, and
frame pointers (or an unwind-table alternative) are three separate,
independently controllable things, and a profile's quality is bounded by
whichever of them is missing — not by the profiler's capability.

## Key Takeaway

**A profiler can only reconstruct the call path from the metadata and
unwinding evidence the build and runtime make available.**

## What to Remember

- A symbol table maps addresses to names; debug information separately
  maps addresses to source file and line — a binary can have either
  without the other.
- A stripped binary has had some or all of this metadata deliberately
  removed, usually to save space.
- Stack unwinding reconstructs the caller chain from a single snapshot,
  commonly via frame pointers or, where supported, DWARF-based or
  last-branch-record-based alternatives.
- Omitting frame pointers is a real, legitimate optimization tradeoff,
  not a mistake — whether it's worth it depends on whether reliable
  profiling of that binary matters more than the resources it costs.
- JIT-generated code and container filesystem boundaries are two further
  ways symbol availability can break down, independent of the build's
  own debug settings.
- Missing or wrong symbols are almost always a property of what the
  build and deployment provided, not a defect in the profiling tool.

## Further Reading

- DWARF Debugging Standard: <https://dwarfstd.org>
- Linux perf manual pages (`--call-graph` options):
  <https://man7.org/linux/man-pages/man1/perf.1.html>

## The Next Obvious Question

How do flame graphs show the shape of work?
