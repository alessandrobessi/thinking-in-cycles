# Appendix B — `perf` Command Map

**Status:** reference material, not a chapter — a compact
question-to-command guide, not a replacement for the main narrative.
Every command below is documented against `perf`'s stable interface
per the same policy used throughout Chapters 10, 12, 13, 15, and 20:
this book's own reference machine is macOS, so these are **schematic,
not tested against real captured output here** — each links back to
the chapter that explains what the command actually measures, what it
doesn't, and its permissions and overhead.

| Question | Command | Chapter |
|---|---|---|
| Is this workload CPU-bound, and on what? | `perf stat -- <cmd>` | 10 |
| What's the IPC, and is it healthy for this workload? | `perf stat -e instructions,cycles -- <cmd>` | 7, 10 |
| Which specific hardware events do I have available? | `perf list` | 10 |
| Are my counters being multiplexed (and is that hiding something)? | `perf stat -e '{...}'` (event group) | 10 |
| Which functions consume the most CPU time? | `perf record -F 99 -g -- <cmd>` then `perf report` | 11, 12 |
| Show me the same data as text, sorted by self cost | `perf report --stdio --sort=overhead,symbol` | 12 |
| Annotate one hot function down to source/assembly lines | `perf annotate <symbol>` | 12 |
| Why are my call stacks broken or showing `[unknown]`? | `perf record -g --call-graph=dwarf -- <cmd>` | 13 |
| Turn a profile into a flame graph | `perf script \| stackcollapse-perf.pl \| flamegraph.pl > out.svg` | 14 |
| Prove an optimization actually helped, not just moved cost | `perf record` before and after, then `perf diff` (or this book's `flamegraph_svg.py --diff-against`) | 15 |
| Where does cache/memory time actually go? | `perf mem record -- <cmd>` then `perf mem report` | 20 |
| Which cache line is contended across cores/sockets? | `perf c2c record -- <cmd>` then `perf c2c report` | 20 |
| List available tracepoints for a subsystem | `perf list 'sched:*'` (or the subsystem's own prefix) | 26 |
| Trace a specific tracepoint's events directly | `perf trace -e sched:sched_switch` | 21, 26 |
| Attach a dynamic probe to an arbitrary kernel function | `perf probe --add <function>` then `perf record -e probe:<function>` | 26 |
| What's this process's run-queue latency? | `perf sched record -- <cmd>` then `perf sched latency` | 21 |
| Am I even allowed to run any of this unprivileged? | `cat /proc/sys/kernel/perf_event_paranoid` | 10 |

## Reading the table

Every row above is a *starting point*, not a complete invocation —
real usage needs a target (`-- <cmd>` or `-p <pid>`), a duration or
event count, and usually a specific event list once the default
generic events stop being precise enough (Appendix C). The chapter
column is where the *why* lives: what the command measures, what it
doesn't, what it costs, and what commonly goes wrong — this table
exists purely to answer "which command do I even start with," not to
replace reading that chapter's Tool View section before running
anything against a system that matters.

## Related

- Section 15's tool-selection ladder (counting before sampling before
  tracing) governs which row of this table to reach for first.
- Appendix A (permissions, kernel/`perf` version matching).
- Appendix C (which events are actually available once you get past
  the generic aliases this table assumes).
