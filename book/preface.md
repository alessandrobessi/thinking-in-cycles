# Preface

Linux performance engineering is usually taught as a bag of commands: run
`top`, try `perf`, generate a flame graph, inspect cache misses, pin a
process, reach for eBPF when everything else fails. That approach
produces engineers who know tools without knowing what question each tool
actually answers, what its measurements mean, or when its output is
quietly misleading them.

This book takes the opposite approach. It builds one cumulative mental
model of where time goes in a Linux system — from a workload creating
work, through the scheduler placing that work on CPUs, through
instruction pipelines, caches, and memory, through threads competing for
shared resources, to the tools that observe all of it, each with its own
blind spots. The goal isn't to make you fluent in more commands. It's to
get you from a vague complaint — "this is slow" — to a testable
hypothesis, the right measurement, a defensible interpretation of the
evidence, one change, and proof of whether it helped.

Every chapter in this book follows the same discipline: introduce the
phenomenon before the tool, prediction before measurement, evidence before
advice. A command should feel like the answer to a question you already
have, not a spell you're told to cast. Tuning advice — "pin the process,"
"disable SMT," "use huge pages" — is treated throughout as a hypothesis
with costs and failure modes, never as folklore to apply on faith.

The book is organized around four principles:

1. Measure a defined workload, not an abstract program.
2. Form a hypothesis before reaching for a specialized tool.
3. Interpret measurements through a model of the hardware and operating
   system.
4. Treat every optimization as an experiment that can fail, regress, or
   move the bottleneck.

You'll see one recurring lab tool throughout — `cyclelab` — a small
command-line workload generator with modes that expose different
performance phenomena, so that each new chapter builds on a codebase
you already understand instead of asking you to learn a new one. As of
this edition, only `cyclelab`'s `compute` mode is implemented; the rest
are recognized by the tool but not yet built, and later parts of the book
will build them as they're needed.

By the end of this book, "the system is slow" should no longer feel like
a mysterious complaint. It should feel like the beginning of a structured
investigation — one you know how to run yourself.

---

*This is a pre-publication manuscript. As of this edition, Part I
(Chapters 1-5) is drafted; Parts II through VI and the appendices are
not yet written. See `ROADMAP.md` for current status.*
