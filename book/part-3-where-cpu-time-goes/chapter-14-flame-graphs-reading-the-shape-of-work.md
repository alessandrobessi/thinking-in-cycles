# Flame Graphs: Reading the Shape of Work

**Part:** Part III — Where the CPU Time Goes
**Concept level:** 3
**Prerequisites:** call graph, inclusive versus self cost, symbol table (Chapters 12-13)
**New concepts:** folded stack, stack aggregation, frame width, ancestry, plateau, tower, CPU flame graph, off-CPU flame graph

## Opening Question

How do flame graphs show the shape of work?

## Incident or Real-World Story

A new engineer is handed a flame graph during an incident review and,
trying to look useful under time pressure, points confidently at the
widest, brightest-colored frame near the middle of the image: "that's
obviously where all the time is going, right there." A more experienced
teammate points out two things at once: the "brightest" color is an
artifact of the rendering tool's palette assignment, not a magnitude
scale — an adjacent frame with nearly identical width is a duller color
purely because of how the two function names happened to hash — and the
wide frame in question is a wrapper that calls three other functions
beneath it, each occupying most of its width. The wrapper's own code
does almost nothing; the width is being passed through from its
children.

Nobody in the room was acting in bad faith. Flame graphs pack an
enormous amount of information into a compact, visually striking image,
and that's exactly what makes them easy to misread quickly — width,
height, and color each mean something specific, and mixing them up
produces a confident, wrong answer just as easily as a correct one.

## Predict Before Measuring

Before reading further — and before looking at this chapter's prepared
figure below — predict: in a CPU flame graph of a program with two
independent worker threads, each doing unrelated work, would you expect
their frames to appear stacked on top of each other, side by side, or
interleaved? What would determine which one appears further left?

## Worked Example

Study the flame graph below before generating your own — this is real,
captured data (not a schematic illustration), from `cyclelab compute
--threads=2 --chains=4 --op=mixed` sampled with macOS `sample` on the
reference machine for this book, folded and rendered by the tools this
chapter's lab uses directly (`labs/scripts/foldstacks.py` and
`labs/scripts/flamegraph_svg.py`).

![A flame graph of cyclelab compute with two worker threads. A broad "thread" base spans the full width. The left half rises through start, main, compute_run, _pthread_join, to a plateau in __ulock_wait. The right half rises through thread_start, _pthread_start, to a wide plateau in compute_worker, with a narrow sliver rising further into timing_now_seconds and its callees.](../../figures/generated/ch14-flame-graph-example.svg)

*Figure: `cyclelab compute --threads=2 --chains=4 --op=mixed`, sampled
for 3 seconds with macOS `sample`, folded with `foldstacks.py`, rendered
with `flamegraph_svg.py`. Reference machine: Apple M4, macOS, arm64.*

Reading it deliberately, bottom to top, left to right:

- **Broad base frames:** the bottommost `thread` frame spans the full
  width — every sample landed inside *some* thread, unsurprisingly.
- **Separate execution towers:** above that base, the graph splits into
  two distinct towers side by side — the left one rooted at `start`
  (the main thread, which spends the whole run blocked in
  `_pthread_join` → `__ulock_wait`), the right one rooted at
  `thread_start` (the worker thread actually doing the arithmetic). They
  sit side by side because the renderer sorts siblings by name, *not*
  because one ran before the other (Predict Before Measuring, answered).
- **Wrapper functions:** `thread_start` and `_pthread_start` are exactly
  as wide as `compute_worker` beneath them — they call straight through
  to it and do essentially no work of their own. Their width is
  inherited from their child, not earned by their own code.
- **Leaf-heavy CPU work:** `compute_worker` is a wide **plateau** — a
  frame with little to nothing rising above most of its width — meaning
  most of its own samples are self cost, not further calls.
- **A narrow tower:** a thin sliver rises from a small fraction of
  `compute_worker`'s width up through `timing_now_seconds`,
  `clock_gettime`, and several further system frames — the periodic
  deadline check, real but small, exactly the kind of narrow-but-real
  path Chapter 11 warned a sampling view can barely catch at all.

## Core Intuition

A **folded stack** is one line of text representing one unique call
path and how many samples landed there — `frame1;frame2;frame3 count` —
the intermediate format between raw captured samples and a rendered
image. **Stack aggregation** is the process of merging every sample that
shares an identical call path into that one folded line, which is what
makes a flame graph a *summary* rather than a list of individual
events — the canonical picture is a population map of observed call
stacks, not a chronicle of any single one; never call it a timeline.
**Frame width** is the only dimension that encodes magnitude — it's
proportional to that frame's share of total samples. **Ancestry** is a
frame's chain of parents going down to the root — everything directly
below a given frame in the same tower. A **plateau** is a wide frame
with little rising above it (self-cost-heavy); a **tower** is a full
vertical stack of frames from base to top, representing one call path's
full depth. A **CPU flame graph** is built from on-CPU sampled stacks
(this chapter's kind); an **off-CPU flame graph** is built instead from
time spent blocked or waiting, requiring an entirely different capture
method (Chapter 29) since on-CPU sampling structurally cannot see it.

## Technical Explanation

Height and color are easy to over-interpret, and both have narrower
meanings than width. **Height** encodes only stack depth — how many
frames deep a given sample's call path was — not time, not cost, not
anything cumulative; a tall, narrow tower and a short, wide plateau can
represent wildly different amounts of total time despite the eye being
drawn to height first. **Color**, in the renderer this book uses (and in
most flame graph tools by default), is assigned per frame name for
visual distinction between neighbors — not a second encoding of
magnitude. A brighter or "hotter"-looking color does not mean more
samples; only width does that. Differential flame graphs (Chapter 15)
are the one case where color *does* carry meaning — there, it encodes
change relative to a baseline, a deliberately different and explicitly
labeled convention, not the default.

The canonical generation pipeline keeps three stages explicit, matching
the capture → fold → render model this book's own tooling mirrors:

```bash
# Linux, canonical form (documented, not tested on this book's macOS
# reference machine):
perf record -F 99 -g -- ./labs/cyclelab/bin/cyclelab compute --duration=3 --threads=2
perf script > out.perf
stackcollapse-perf.pl out.perf > out.folded
flamegraph.pl out.folded > flame.svg
```

This book's portable pipeline (`capture_sample_profile.sh` →
`foldstacks.py` → `flamegraph_svg.py`) produces the same intermediate
folded-stack format at the middle stage, which is exactly why a
`.folded` file from either pipeline renders correctly in either
renderer — the format, not the specific tools, is the actual interface.

## Tool View

- What is measured: this chapter's lab captures, folds, and renders a
  real flame graph from a `cyclelab` run.
- What is not measured: an off-CPU flame graph (Chapter 29) or a
  differential flame graph (Chapter 15, next) — both use this same
  folded-stack format but a different capture method or a second input
  file, respectively.
- Required permissions: none for this chapter's lab.
- Likely overhead: sampling overhead only, as in Chapters 11-12.
- Portability: `labs/scripts/flamegraph_svg.py` accepts a folded-stack
  file from *either* this book's macOS pipeline or a real
  `stackcollapse-perf.pl` output on Linux — the renderer doesn't care
  which produced it. This book's renderer is deliberately minimal
  (static SVG, no hover/zoom/search) compared to the real FlameGraph
  project (<https://github.com/brendangregg/FlameGraph>), which is worth
  using directly for any real investigation.
- Common failure mode: reading height or color as magnitude (this
  chapter's incident, directly), or reading left-to-right position as
  chronological order (M06).

## Guided Lab

**Portability:** portable.

**Setup:** build `cyclelab` if you haven't already (`make lab-cyclelab`).
Requires macOS's built-in `sample`(1) and `python3`.

**Command:**

```bash
./labs/scripts/capture_sample_profile.sh /tmp/my_flame.folded 3 -- \
  compute --duration=4 --threads=2 --chains=4 --op=mixed --quiet --output=/dev/null
python3 labs/scripts/flamegraph_svg.py /tmp/my_flame.folded -o /tmp/my_flame.svg
open /tmp/my_flame.svg   # macOS; use xdg-open or a browser directly on Linux
```

**Expected qualitative result:** a flame graph with the same overall
shape as this chapter's prepared figure — a broad base, two side-by-side
towers (main thread blocked, worker thread computing), a wide
`compute_worker` plateau, and a thin sliver rising into the timing
check — though exact widths and sample counts will differ run to run.

**Interpretation:** work through the same reading exercise as the Worked
Example, on your own output: find the broadest base frame, identify
which tower is leaf-heavy (a plateau) versus wrapper-heavy (width passed
straight through to one child), and find the narrowest real path you can
still identify. If your run's shape looks meaningfully different from
the prepared figure — for instance, if the two towers aren't
roughly proportioned the way this chapter's example shows — that's
itself worth investigating with what Chapters 11-13 already gave you,
rather than assumed to be an error.

**Fallback path:** if `sample` isn't available, study the prepared
figure above on its own — every reading skill this lab exercises (base
frames, plateaus, wrappers, towers, magnitude-vs-height-vs-color) can be
practiced against it directly, since it's real captured data, not a
schematic stand-in.

**Cleanup:** none.

## Common Misconceptions

**M06 — "A flame graph is a timeline."** This is wrong because a flame
graph aggregates stack samples by call path; horizontal position comes
from sibling sort order (typically alphabetical), not chronology. The
evidence that distinguishes the two: this chapter's own figure has two
towers side by side from two threads that ran concurrently for the
entire capture window — neither one "happened first."

**M07 — "The widest frame is the function to optimize."** This is wrong
because width can represent a wrapper passing cost through to a child,
not necessarily the frame's own expensive code. The evidence that
distinguishes the two: `thread_start` and `_pthread_start` in this
chapter's figure are exactly as wide as `compute_worker` beneath them,
yet do essentially none of the actual work themselves.

**M31 (proposed) — "A flame graph's height or color encodes cost."**
This is wrong because only width encodes magnitude; height is stack
depth, and color (outside a differential flame graph) is only a visual
distinguisher between neighbors. The evidence that distinguishes the
two: two equally-wide frames at the same depth can render in visibly
different colors purely from how their names hash into the palette.

**M32 (proposed) — "A narrow frame can't matter to tail latency."**
This is wrong because a CPU flame graph only shows on-CPU sampled
execution (M08); the rare, high-impact event from Chapter 11's opening
story would show up, at best, as a barely-visible sliver here — or not
at all, if it's off-CPU entirely. The evidence that distinguishes the
two: this chapter's own thin `timing_now_seconds` sliver is real and
small; a tail-latency-dominating off-CPU event would be smaller still,
or invisible, in this exact kind of graph.

## Practical Implications

Before drawing a conclusion from a flame graph, check three things in
order: is the frame you're looking at a plateau (self-cost-heavy) or a
wrapper (child-cost-heavy)? Does its width — not its height, not its
color — actually represent a meaningful share of the total? And is the
question you're asking even answerable from a CPU flame graph, or does
it live off-CPU where this view structurally can't see it?

## Key Takeaway

**A flame graph compresses sampled stacks into width: it shows where
execution accumulated in the call hierarchy, not a timeline.**

## What to Remember

- Folded stacks (`frame1;frame2;... count`) are the shared intermediate
  format between capture and rendering, portable across toolchains.
- Width is the only dimension that encodes magnitude; height is stack
  depth; color (outside differential mode) is only a visual
  distinguisher.
- Sibling frames are ordered by name, not by execution time — flame
  graphs are not timelines.
- A plateau (wide, little above it) suggests real self cost; a wrapper
  (width passed straight to one child) suggests looking one level
  deeper instead.
- Separate towers commonly represent separate threads or separate call
  paths active during the same capture window, not sequential events.
- A CPU flame graph structurally cannot show off-CPU time — a narrow or
  absent frame here says nothing about whether that path matters for
  tail latency.

## Further Reading

- Brendan Gregg, "The Flame Graph," *Communications of the ACM*, 2016 —
  the original explanation of the technique from its creator.
- FlameGraph project: <https://github.com/brendangregg/FlameGraph>

## The Next Obvious Question

How do we prove that an optimization changed the right thing?
