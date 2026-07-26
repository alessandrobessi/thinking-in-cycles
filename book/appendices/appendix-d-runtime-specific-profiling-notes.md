# Appendix D — Runtime-Specific Profiling Notes

**Status:** reference material, not a chapter. Chapters 11-15's
profiling model (counting, sampling, tracing, stacks, symbols, flame
graphs) is language-agnostic by design; this appendix collects the
specific ways each common runtime helps or hurts that model, without
repeating the underlying mechanism those chapters already cover.

## C/C++

The reference case this entire book is built against — `cyclelab`
itself is C11, compiled with `-g` and (in the debug build) `-O0`,
frame pointers intact by default. Native, ahead-of-time-compiled code
with standard debug information is exactly what Chapter 13's symbol and
unwinding model assumes: a stripped binary loses symbols but keeps
correct unwinding if frame pointers or `.eh_frame`/DWARF CFI data
remain; an optimized build (`-O2`) can still symbolize correctly but may
show inlined functions collapsed into their caller, or a hot loop's
disassembly reordered from the source. Nothing in this appendix's other
entries is easier than this one — it's the baseline every other
runtime's extra notes below are relative to.

## Rust

Profiles essentially like C/C++, for the same reason: Rust compiles
ahead-of-time to native code via LLVM, and standard symbol/unwinding
tooling works the same way once debug info is present (`debuginfo =
true` in a release profile, since Rust's default release build strips
it). The one Rust-specific wrinkle worth flagging: heavy use of
generics and trait objects can produce long, heavily-mangled symbol
names in a raw profile (`demangle`-aware tooling, which most modern
profilers include by default, handles this automatically) and can
inline aggressively enough that a flame graph's frame boundaries
represent the *optimizer's* view of function boundaries more than the
source's.

## Go

Go's runtime schedules many lightweight **goroutines** onto a smaller
number of OS threads, which changes what a stack sample actually shows:
a single OS-thread-level profile can multiplex many logically distinct
goroutines' stacks, and Go's own `pprof` tooling — not a generic
system profiler — is the practical way to get a goroutine-aware view.
`pprof`'s own model (CPU profile, heap profile, goroutine dump) maps
onto this book's counting/sampling/tracing framework directly (Chapter
11): a CPU profile is periodic sampling; a heap profile is closer to an
event-triggered trace of allocation sites. Go binaries are statically
linked and include their own runtime symbols by default, so basic
symbolization is usually straightforward even without extra debug-info
flags.

## JVM

The JVM presents the sharpest version of Chapter 13's stack-unwinding
problem: bytecode running under a **just-in-time (JIT) compiler**
means the actual native code being executed didn't exist at process
start and can be re-compiled or de-optimized mid-run, so a generic
system profiler walking raw instruction pointers has nothing stable to
map addresses to unless it specifically understands the JVM's own
symbol-publishing mechanism (`perf-map-agent` and similar tools exist
specifically to bridge this gap on Linux, publishing a
`/tmp/perf-<pid>.map` file the profiler can consult). JVM-native
profilers (async-profiler and similar) sidestep the problem by hooking
the JVM's own profiling APIs directly rather than sampling raw
instruction pointers from outside, at the cost of being JVM-specific
rather than general-purpose.

## Python

CPython's default interpreter loop means a generic system profiler
mostly sees the interpreter's own C-level frames (`PyEval_EvalFrameEx`
and similar) repeated over and over, not the Python-level call stack a
developer actually wants — the interpreter itself is the stable, native
part Chapter 13's model handles fine; the *Python* stack sitting on top
of it needs a Python-aware profiler (`py-spy`, `austin`, or the
standard library's own `cProfile`/`profile` for in-process, lower-fidelity
use) to be visible at all. Recent CPython versions (3.12+) have begun
exposing native, lower-overhead sampling support directly, narrowing
this gap, but the underlying lesson generalizes beyond Python: any
interpreted or bytecode-VM language needs a profiler that understands
*that* runtime's own frame representation, not just native stack
unwinding.

## JIT symbolization

The general version of the JVM's problem: any runtime that generates
native code at runtime (JVM, JavaScript engines like V8, `.NET`'s CLR)
breaks a generic profiler's assumption that "what's mapped at this
address" is knowable from the binary's own static symbol table alone.
The common fix pattern, regardless of runtime, is the same shape as
`perf-map-agent`: the runtime (or an agent attached to it) publishes a
side-channel address-to-symbol map the profiler can consult, refreshed
as code is compiled and discarded. A profile that shows large stretches
of `[unknown]` or raw hex addresses in an otherwise well-symbolized
stack is the direct symptom to watch for — Chapter 13's own guided lab
demonstrates the non-JIT version of exactly this signature (a stripped
binary versus a full-debug one) on native code.

## Frame pointers and runtime profilers

Chapter 13's frame-pointer-omission experiment (native code, `-g` vs.
no `-g`, `-fomit-frame-pointer` vs. not) is the general case; most
managed runtimes ship their own frame-pointer-equivalent bookkeeping
internally (the JVM's stack walker, Go's own runtime stack maps) so
that runtime's *own* tooling can unwind correctly even when the
underlying native code was built without traditional frame pointers.
The practical implication: a general system profiler's ability to
unwind through a managed runtime's frames often depends on flags
specific to that runtime (JVM's `-XX:+PreserveFramePointer` is the
canonical example — off by default on some JVM builds, specifically
because it exists to help *external* native profilers unwind through
JIT-compiled frames, at a small steady-state performance cost) rather
than anything the system profiler itself controls.

## Related

- Chapter 13 (symbols, unwinding, frame pointers — the language-agnostic
  mechanism every entry above is a specific runtime's version of).
- Chapter 11 (counting/sampling/tracing as the three observation models
  each runtime-specific profiler above still fundamentally implements
  one of).
- Appendix B (the underlying `perf` commands these runtime-aware tools
  often wrap or replace for their specific runtime).
