# From Source Code to Retired Instructions

**Part:** Part II — What the CPU Is Doing
**Concept level:** 2 (Level 0-1 concepts from Part I assumed; formally begins the CPU-work concept level)
**Prerequisites:** workload, on-CPU/off-CPU accounting, the investigation loop (Chapters 1-5)
**New concepts:** compiler, machine instruction, micro-operation intuition, retired instruction, optimization, vectorization, inlining, dead-code elimination

## Opening Question

What work does the CPU actually execute?

## Incident or Real-World Story

A developer is asked to speed up a hot function and responds by manually
"simplifying" it: combining several short statements into fewer, denser
lines, on the theory that less source code means less work for the CPU.
The refactored version reads more compactly. A teammate benchmarks it out
of habit before merging and finds no measurable difference in either
direction — same instruction count, same runtime, same everything, down
to the compiled machine code being close to byte-identical in the
function's hot loop.

Nothing was wrong with the refactor; it just never had a chance to change
anything, because the compiler had already been doing the same
transformation the developer was trying to do by hand. The CPU was never
going to execute the *source code* either way — it executes whatever
machine instructions the compiler decided to emit, and a sufficiently
capable compiler at a reasonable optimization level had already reordered,
combined, and pruned the arithmetic before either version of the source
ever reached it. The lesson isn't "don't bother writing clear code." It's
that visual density in source code and the amount of work the CPU
actually does are only loosely related, and conflating the two leads to
refactors that change nothing.

## Predict Before Measuring

Before reading further: if you compile the exact same C function once
with no optimization (`-O0`) and once with a standard optimization level
(`-O2`), do you expect the resulting machine code to contain about the
same number of instructions, meaningfully fewer, or meaningfully more at
`-O2`? Hold that prediction — the Guided Lab checks it directly, on a
function you already have the source for.

## Worked Example

Compare an unoptimized and an optimized build of the same loop-carrying
function — not a rewritten version, the *identical source*, compiled
twice. At `-O0`, a compiler typically emits close to a literal,
line-by-line translation: every intermediate value gets written to and
read back from the stack, because the compiler isn't tracking whether
that round-trip is necessary — it's optimized for fast, predictable
compilation and easy debugging, not for a small or fast binary. At
`-O2`, the same source can produce a function that keeps values in
registers across many operations, reorders independent work, and removes
computations whose results are never used. The two binaries can behave
identically as far as the program's observable output goes, while being
substantially different pieces of machine code underneath — which is
exactly why "read the source" and "know what the CPU executes" are
different skills.

## Core Intuition

A **compiler** translates source code into **machine instructions** — the
literal, architecture-specific operations (load this, add that, branch
here) a CPU can execute. Source-level structure (which statements are
grouped together, how densely code is written) does not map one-to-one
onto that instruction stream; the compiler is free to reorder, merge, and
delete work as long as the program's observable behavior doesn't change.
A **retired instruction** is one that has actually completed execution
and had its effect committed — the thing performance counters count when
they report "instructions," as later chapters will use directly.
**Optimization** is the general term for any transformation a compiler
applies to make the emitted instructions do the same job with less work:
**inlining** (replacing a function call with the callee's body directly,
removing call overhead and opening the door to further optimization
across the former call boundary), **vectorization** (using instructions
that operate on multiple data elements at once instead of one at a
time), and **dead-code elimination** (removing computations whose
results provably can't affect the program's observable output) are three
of the most consequential ones for performance work specifically.

## Technical Explanation

Modern CPUs additionally break most machine instructions down further
into **micro-operations** internally before executing them — this is
worth knowing as a name and nothing more at this stage; the pipeline
mechanics that make micro-operations matter are Chapter 8's subject, not
this one. For now, the actionable idea is narrower: the compiler's
output — the machine instruction stream — is the real subject of every
later chapter in this Part, not the source text a programmer reads. Two
functions that look very different in source can compile to nearly
identical machine code (as in this chapter's incident), and two
functions that look nearly identical in source can compile to very
different machine code, if one of them defeats an optimization the other
allows (a function called through a pointer, for instance, often can't be
inlined the way a directly-called one can).

This is also the first moment `cyclelab`'s own build setup becomes worth
looking at directly rather than taking on faith: it deliberately builds
both a `-O0 -g` ("debug") and a `-O2 -g` ("release") variant of the exact
same source, specifically so this comparison is always available
(`labs/cyclelab/Makefile`).

## Tool View

- What is measured: the compiled instruction stream for one function, at
  two optimization levels, via disassembly.
- What is not measured: dynamic behavior — a disassembly listing shows
  the *static* instructions in a function's compiled body, not how many
  times each one actually executes, nor how long any of them take. That
  connection is Chapter 7's subject.
- Required permissions: none.
- Likely overhead: none — this is offline inspection of an already-built
  binary, not a running measurement.
- Portability: `objdump -d` is available via binutils on Linux and via
  Xcode Command Line Tools on macOS, but the exact invocation differs
  slightly by platform (macOS's `objdump` needs `--macho` to select the
  right disassembly mode); symbol names and register conventions differ
  entirely between x86-64 and Arm64. `perf annotate` (Linux-only, and
  requiring a captured `perf record` profile) is the eventual, richer
  version of this same idea, covered in Part III.
- Common failure mode: comparing disassembly of two *different* source
  versions and attributing every difference to the change under test,
  when compiler version, flags, or unrelated code nearby can also shift
  the output.

## Guided Lab

**Portability:** portable — no root, no `perf`. Requires `objdump`
(present on Linux with binutils installed, and on macOS with Xcode
Command Line Tools installed).

**Setup:** none beyond having `objdump` available; the script builds
both `cyclelab` variants itself.

**Command:**

```bash
./labs/scripts/ch6_build_and_disassemble.sh
```

This builds `cyclelab`'s debug (`-O0 -g`) and release (`-O2 -g`)
variants and disassembles the same function, `compute_worker` — the core
loop from Chapter 1 onward — from each, comparing static instruction
count and showing the first several instructions of each side by side.

**Expected qualitative result:** the release build's version of the
function should show fewer static instructions than the debug build's,
and its first several instructions should show values moving between
registers more and the stack less. One example run on the reference
machine for this book (Apple M4, macOS, arm64, Apple LLVM 17 / clang)
showed:

```text
compute_worker() static instruction count:
  -O0 (debug):   336 instructions
  -O2 (release): 192 instructions
```

**Interpretation:** do not expect these exact counts, or even
necessarily this exact ratio, on a different architecture or compiler
version — x86-64 output will use entirely different mnemonics and
register names than the Arm64 example above, and a newer or older
compiler can make different tradeoffs. The qualitative result — the
same source, compiled twice, producing a smaller, more register-heavy
instruction stream at the higher optimization level — is what to look
for, not the specific numbers.

**Fallback path:** if `objdump` isn't available on your system, the same
qualitative point can be observed without disassembly at all: build both
variants (`make lab-cyclelab` from the repo root) and run `ls -la
labs/cyclelab/bin/cyclelab-debug labs/cyclelab/bin/cyclelab` — even
comparing whole-binary file sizes between the two builds is weak but
real evidence that the compiler produced meaningfully different output
for the same source.

**Cleanup:** the script removes its own temporary files.

## Common Misconceptions

### *"Fewer instructions always means faster code." (M03)*

**Why it's wrong:** Instruction count is only one factor; stalls, vector
width, memory behavior, and clock frequency also determine how long a
sequence of instructions actually takes to retire — a chapter this book
returns to directly in Chapter 7.

**Correct intuition:** Compare two functions' instruction counts *and*
their measured elapsed time under the same conditions; a function with
more instructions can still finish faster if its instructions are
cheaper or better pipelined.

**Analogy:** A ten-item to-do list of quick errands can finish faster
than a three-item list that includes "wait in line at the DMV" — the
number of items on the list says nothing about how long each one
actually takes.

### *"Visually denser source code means less CPU work."*

**Why it's wrong:** A compiler at a reasonable optimization level is
already applying most of the same transformations a programmer might
attempt by hand-simplifying source — the compiled instruction stream,
not the source text's density, is what determines CPU work.

**Correct intuition:** Disassemble both versions of a "simplified"
function, as in this chapter's Guided Lab, and check whether the machine
code actually changed — often it hasn't, because the compiler already
got there first.

**Analogy:** Rewriting a recipe in shorter sentences doesn't change how
long the dish takes to cook — the words on the page and the actual
cooking process are two different things, and a good chef (the
compiler) was already ignoring your prose style and going straight to
the technique.

## Practical Implications

Before assuming a source-level change will affect performance, ask
whether it changes the compiled instruction stream at all — many
"readability" refactors don't, because the compiler was already
producing equivalent code either way. When a change *is* expected to
matter, disassembly (or, more practically once Part III arrives,
profiling) is what confirms the compiler actually did what you expected,
rather than trusting the source diff alone.

## Key Takeaway

**The CPU executes the compiler's instruction stream, not the
programmer's visual impression of the source.**

## What to Remember

- The compiler, not the programmer, decides the final machine
  instruction stream, within the bounds of preserving observable
  behavior.
- Source-level density (fewer, more compact lines) does not reliably
  predict compiled instruction count.
- Inlining, vectorization, and dead-code elimination are three of the
  most consequential optimizations for performance work specifically.
- A retired instruction is one that actually completed and committed its
  effect — the unit later chapters' counters measure directly.
- Micro-operations are a real further layer beneath machine instructions,
  worth knowing by name now; their mechanics are Chapter 8's subject.
- Disassembly shows static structure, not dynamic behavior — how many
  times each instruction actually executes is a different question.
- `cyclelab` intentionally ships both a `-O0` and a `-O2` build so this
  comparison is always available without extra setup.

## Further Reading

- Compiler Explorer (<https://godbolt.org>) — the standard interactive
  tool for exploring how source changes affect compiled output across
  compilers and architectures; not required for this chapter's offline
  lab, but the natural next step for readers who want to keep exploring.
- `objdump` manual page — `man objdump` on Linux; Apple's `objdump`
  ships its own man page via Xcode Command Line Tools.

## The Next Obvious Question

What do cycles and instructions tell us?
