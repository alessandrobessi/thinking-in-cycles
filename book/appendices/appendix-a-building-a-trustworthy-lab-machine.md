# Appendix A — Building a Trustworthy Lab Machine

**Status:** reference material, not a chapter. No Opening Question,
Guided Lab, or Key Takeaway. Cross-refers to chapters throughout rather
than repeating their content.

This appendix collects, in one place, the machine-level decisions that
determine whether any measurement taken elsewhere in this book can be
trusted at all. None of it is new material — every point below is
already implied by some chapter's Tool View or Guided Lab section — but
it's worth having as a single pre-flight checklist before a real
investigation, rather than rediscovered piecemeal one caveat per
chapter.

## Bare metal versus VM

This book's own portability policy (quoted throughout wherever a lab
is tagged) is blunt about virtualization's cost to measurement fidelity:
PMU events may be hidden, incomplete, or virtualized; cycle counts may
behave differently under a hypervisor's own scheduling; steal time and
host interference add noise no amount of repetition removes; NUMA
topology may be synthetic rather than reflecting real hardware; uncore
and memory-controller counters may be unavailable entirely. None of
this makes a VM useless — it makes a VM a *different measurement
environment*, one whose numbers don't transfer to bare metal without
re-verification. Chapters tagged **bare-metal recommended** (Chapters
19-20's bandwidth/uncore material, Chapters 23-25's topology material)
are exactly the chapters where this gap is largest; chapters tagged
**portable** (most of Parts I-III) are largely insulated from it,
because they measure relationships (does A change relative to B) rather
than absolute hardware-specific numbers.

## Matching `perf` to the kernel

`perf`'s own version is tied to the kernel it ships alongside, and a
mismatched pair (a newer `perf` against an older running kernel, or vice
versa) can silently drop support for specific events, subcommands, or
output fields without necessarily erroring outright. `make doctor`'s
"perf version" check (Linux only) reports both the running kernel
release and `perf --version`'s own report side by side specifically so
a mismatch is visible before it causes a confusing, silent gap in
Chapter 10 or 12's counter data. When in doubt, prefer the `perf` binary
that shipped with the running kernel's own package repository over a
manually built or backported one.

## Debug symbols

Chapter 13's entire subject is what happens when this goes wrong: a
stripped binary, or one built without frame pointers, can turn a
perfectly good profile into an unreadable stack of hex addresses and
`[unknown]` frames. The practical rule Chapter 13 arrives at: build lab
and target binaries with debug symbols retained (`-g`) and, unless
there's a specific reason to omit them, frame pointers preserved
(avoid `-fomit-frame-pointer`, or use `-fno-omit-frame-pointer`
explicitly) — the small runtime cost of keeping a frame pointer around
is almost always worth the difference between a readable and an
unreadable profile. This book's own `cyclelab` builds both variants with
`-g` unconditionally (`labs/cyclelab/Makefile`), precisely so every
chapter's lab has real symbols to work with.

## Permissions and capabilities

Several tools this book documents — `perf` beyond basic counting,
kprobes/kretprobes/uprobes (Chapter 26), eBPF program loading (Chapter
27), most of `bpftrace`/BCC (Chapter 28) — need root or specific
elevated capabilities on Linux, gated further by
`perf_event_paranoid` (Chapter 10) and `kptr_restrict` (Chapter 13).
`make doctor` reports both values directly. This book's own reference
machine demonstrates the same principle from the opposite direction:
macOS's `dtrace`, structurally comparable to Linux's dynamic-tracing
tools, is present but refuses to run unprivileged under this machine's
System Integrity Protection policy (Chapter 26) — a real, tested example
of "requires privilege" as a property of the *system's security policy*,
not just the tool.

## CPU governors and turbo

A CPU running under a power-saving governor, or one whose clock is
free to boost opportunistically under turbo, can show run-to-run
throughput variance that has nothing to do with the code being measured
— exactly the kind of confounder Chapter 4 exists to catch. On Linux,
check the active governor (`cpupower frequency-info`, or read
`/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor` directly) and
prefer `performance` over `powersave`/`ondemand` for a benchmark run
where frequency variance would otherwise be mistaken for a code change's
effect. This book's own reference machine (Apple Silicon, macOS) has no
equivalent user-facing governor control at all — the operating system
manages P-core/E-core scheduling and frequency scaling itself, which is
part of why Chapter 4's guided lab treats warm-up and repetition as the
portable defense against this class of noise, rather than a governor
setting only Linux exposes.

## Thermal monitoring

Sustained load can push a CPU into thermal throttling, which looks
identical to a code regression in a benchmark's raw numbers unless
thermal state is tracked alongside performance — Chapter 4's
"confounder" concept applied directly to hardware rather than software.
On a laptop or a compact server especially, a benchmark that runs long
enough to heat-soak the chassis can show declining throughput over its
own duration that has nothing to do with the workload. The practical
defense is the same one Chapter 4 already teaches generally: interleave
configurations rather than running one to completion and then the next,
so a thermal drift affects both configurations' measurements roughly
equally instead of biasing whichever one happened to run second.

## Background services

Any process competing for the same CPUs, memory bandwidth, or cache
capacity as a lab workload is a confounder Chapter 4 would recognize
immediately, and Chapter 22 gives it a name and a real, measured
demonstration: **noisy neighbor** interference. Before trusting a
benchmark's absolute numbers (not just its qualitative shape), check
for background indexing services, scheduled backups, other users' or
containers' workloads, and this book's own lab tooling accidentally
still running from a previous session — `ch22_noisy_neighbor.sh`
demonstrates directly how much a genuinely competing process degrades
an otherwise-identical workload's throughput on this reference machine
(roughly 55-60%, in the real numbers Chapter 22 reports).

## Safe use of root

Several of this book's most capable tools require root, and root
access is also exactly what turns a measurement mistake into a
production incident. Appendix G collects the operational-safety
discipline this implies in full; the short version relevant here:
prefer the least-privileged tool that answers the question (Section
15's tool-selection ladder, followed consistently from Chapter 10
onward), never run an unfamiliar privileged command against a
production system without first understanding its overhead and blast
radius (Chapters 26-28's own repeated cautions about probe rate and
overhead), and treat any data a privileged tool can see — another
process's arguments, memory contents, file paths — with the same care
as you would treat that process's own logs.

## Related

- `make doctor` (`scripts/doctor.sh`) is this book's own executable
  version of this appendix's checklist — run it first on any new
  machine.
- Chapter 4 (noise and confounders), Chapter 10 (`perf_event_paranoid`),
  Chapter 13 (symbols and frame pointers), Chapter 22 (noisy-neighbor
  interference), Chapters 26-28 (privilege requirements for dynamic
  tracing and eBPF), Appendix G (production safety).
