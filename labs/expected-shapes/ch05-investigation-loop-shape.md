# Expected shape: Chapter 5 (`ch5_investigate_slow_config.sh`)

What to expect from comparing an over-threaded `cyclelab compute` run
against a correctly-sized one, in terms of shape, not numbers:

- **Total throughput should be similar, not proportional to thread count.**
  Going from N threads (matching logical CPUs) to 4N threads should *not*
  make `results.throughput_ops_per_s` roughly 4x higher -- there are still
  only N CPUs to execute on. If you see throughput scale linearly with
  thread count in the over-threaded run, re-check that `--threads` was
  actually applied and that the machine wasn't otherwise idle enough to
  have spare CPUs.
- **Per-thread iteration counts should spread out.** In the correctly-sized
  run, each thread's `iterations` in the JSON output should be close to
  every other thread's. In the over-threaded run, expect more spread
  across threads -- some get scheduled more than others over the run's
  duration, because there are more runnable threads than CPUs to run them.
- **`ps`/`vmstat` (or `vm_stat`) should show runnable pressure, not idle
  time.** The point of Step 4-6 is to see that the machine isn't idle
  during the over-threaded run -- CPUs stay busy -- while individual
  threads still make less forward progress each, because they're taking
  turns. That's the "runnable but not running" distinction this chapter
  is building toward Chapter 21's formal scheduler model.
- **The loop, not a single measurement, is the deliverable.** The
  qualitative goal of this lab is to have gone through steps 1-10 of the
  investigation loop (Section 5) with only general-purpose tools, and to
  be able to state a falsifiable hypothesis about the over-threaded run's
  behavior -- not to produce one "correct" number.
