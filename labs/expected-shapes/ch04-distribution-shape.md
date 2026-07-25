# Expected shape: Chapter 4 (`ch4_interleaved_ab.sh`)

What to expect from the interleaved `op=int` vs `op=float` comparison, in
terms of shape, not numbers:

- **Warm-up:** the very first repetition of the whole run is the likeliest
  outlier of the entire table. If it's noticeably slower than the rest,
  that's warm-up (caches, scheduler placement, thermal ramp), not evidence
  about `int` vs `float`.
- **Within-column spread:** repeated measurements of the *same*
  configuration (all the `int` rows against each other, all the `float`
  rows against each other) should cluster fairly tightly on an otherwise
  idle machine. A wide spread within one column is a sign of background
  interference or thermal/frequency effects (Chapter 4's mandatory topics),
  not a property of `int` or `float` arithmetic.
- **Between-column separation:** a real, trustworthy difference between
  `int` and `float` looks like two clusters that are each individually
  tight and don't overlap much. If the two columns' spreads overlap as
  much as either column varies against itself, the apparent difference is
  not distinguishable from noise with this many repetitions -- that's
  Misconception M15 in practice, not a special case of it.
- **Order independence:** because the script alternates `int`/`float`
  rather than running all of one then all of the other, a real background
  interference event (another process waking up, a thermal throttle)
  should show up as a bad value in *both* columns near the same repetition
  number, not as a shift that only ever favors one column.

If your results don't show a clean pattern at all -- that is itself a
valid, useful observation. It usually means the machine wasn't idle enough
for this particular comparison to be conclusive, which is exactly the kind
of judgment Chapter 4 is trying to build.
