# Appendix F — Statistical Reasoning for Benchmarks

**Status:** reference material, not a chapter. Builds directly on
Chapter 4's benchmarking-hygiene material and Chapter 15's
before/after discipline, with the specific statistical vocabulary
those chapters use informally. Every number below is real, generated
for this appendix on this book's own reference machine — not
illustrative placeholders.

## The real data this appendix uses

Two comparisons, twelve repetitions each, `cyclelab compute
--duration=0.3 --threads=1 --op=int`:

**A real effect** (`--chains=1` vs. `--chains=2`):

```text
chains=1: 698.6M 727.9M 728.4M 728.8M 727.5M 729.4M 726.5M 728.8M 726.9M 728.3M 727.4M 729.6M
chains=2: 1342.5M 1355.7M 1347.1M 1343.1M 1340.4M 1342.2M 1344.7M 1339.0M 1338.8M 1334.8M 1337.3M 1329.0M
```

**No real effect** (`--chains=1` run twice, as two separate sessions):

```text
run 1: 711.2M 729.1M 729.0M 730.0M 726.8M 728.9M 726.9M 728.9M 727.6M 728.4M 727.9M 729.0M
run 2: 729.2M 727.5M 728.5M 727.9M 729.1M 727.3M 729.5M 726.8M 729.7M 727.6M 729.0M 727.5M
```

## Medians and percentiles

Chapter 4 already prefers the median and relevant percentiles over the
mean, for the same reason this appendix's own data reinforces: a mean
is more sensitive to a single outlier run (a thermal event, a
background process, one unlucky scheduling decision) than a median is.
The `chains=1` data above has one visible low outlier (698.6M against a
cluster around 727-729.6M), and `run 1` has the same pattern (711.2M
against a 726.8-730.0M cluster) — a mean would shift noticeably toward
each; the median (728.1M and 728.7M respectively) barely moves.
Percentiles generalize this: reporting p50/p90/p99 of a benchmark's own
repeated runs (not just the workload's own latency distribution,
Chapter 3's tail latency) shows whether a "typical" run and a
"worst-case" run are close together or far apart — informative on its
own, before even comparing two configurations.

## Confidence intervals

A **confidence interval** answers a narrower question than it's often
given credit for: not "where is the true value," but "given this
sample, what range of values would be consistent with having produced
it." A **bootstrap** confidence interval builds this range empirically
—resampling the observed data with replacement, many times, and taking
the middle 95% of the resulting distribution of sample means — rather
than assuming a specific theoretical distribution shape. Applied to the
data above (2,000 resamples, 95% interval): `chains=1` gives
[720.6M, 728.6M]; `chains=2` gives [1337.8M, 1344.8M] — no overlap at
all, real evidence the two configurations differ. The same procedure
applied to the two `chains=1` *sessions* — genuinely the same
configuration, run twice — gives [723.9M, 728.8M] and [727.8M,
728.8M]: heavily overlapping, exactly what "no real difference" should
look like. This is the concrete, numeric version of Chapter 4's own
qualitative rule: if repeat runs of the *same* configuration spread
nearly as much as two configurations differ, the difference is not yet
established.

## Bootstrap intuition

The bootstrap's core assumption is that the sample in hand is a
reasonable stand-in for the true underlying distribution — resampling
from it repeatedly simulates "what if I'd gotten a slightly different
set of runs" without needing to assume the data is normally
distributed, symmetric, or any particular shape. This matters directly
for benchmark data, which frequently isn't clean and symmetric: thermal
throttling, scheduler noise, and occasional background interference
(Appendix A) all tend to produce a longer tail on the slow side rather
than a symmetric spread — exactly the kind of distribution shape where
a bootstrap's fewer assumptions are an advantage over a formula that
assumes normality.

## Effect size

A **p-value** or a confidence interval's non-overlap answers "is there
probably a real difference"; **effect size** answers the separate
question "how big is it, in a unit that doesn't depend on sample size."
Cohen's *d* (the difference between two means, divided by their pooled
standard deviation) applied to this appendix's real-effect comparison
gives *d* ≈ 80 — an extreme effect by any conventional threshold
(0.2 small, 0.5 medium, 0.8 large), and *d* this large is exactly what
low run-to-run variance plus a genuinely large mean shift produces: the
pooled standard deviation here (about 7.6M) is tiny relative to the
615.5M-unit gap between the means, so the same gap that shows up as
`--chains=2` running roughly 1.85x the throughput of `--chains=1` also
shows up as a *d* far outside the range effect-size guidance is usually
written for — a reminder that Cohen's thresholds were calibrated on
noisier measurements than a tight, low-variance benchmark repetition
typically produces. The reason effect size matters separately from "is
it real":
a large enough sample can make a genuinely tiny, practically
meaningless difference statistically significant (Chapter 4's own
territory: more repetitions narrow a confidence interval around
*any* true difference, no matter how small), and effect size is what
distinguishes "real but irrelevant" from "real and worth acting on."

## Repeated comparisons and multiple testing

Testing many configurations against a baseline (a parameter sweep, a
grid of flag combinations) and reporting only the ones that "look
significant" is a direct path to false conclusions: with enough
comparisons, some will cross a significance threshold by chance alone,
even with zero real effects present anywhere. Chapter 17's stride
sweep and Chapter 19's thread-count sweep are exactly this situation in
miniature — many configurations measured in one investigation — and
the discipline that protects them is the same one general statistical
practice recommends: look for a **consistent, monotonic pattern**
across the whole sweep (a real mechanism should show up as a smooth
trend, not an isolated spike at one setting) rather than treating each
pairwise comparison in the sweep as an independent, uncorrected test.

## Practical versus statistical significance

The distinction this whole appendix builds toward: a difference can be
statistically well-established (non-overlapping confidence intervals,
a real, reproducible effect) and still not matter for any decision that
depends on it — a real, repeatable 0.3% throughput difference is
"significant" in the statistical sense demonstrated above, but rarely
worth the engineering cost of chasing unless the specific context makes
0.3% valuable at scale. The reverse also holds: a difference too small
to distinguish from noise in ten runs might still be real and
meaningful, just under-measured — more repetitions, not a different
conclusion, is the correct response to that specific situation.
Chapter 4's own discipline (state what the data supports and what it
doesn't, separate observation from recommendation) is this appendix's
statistical vocabulary applied to two sentences instead of a formula.

## Related

- Chapter 4 (repetition, distribution, outliers — the qualitative
  version of this appendix's vocabulary).
- Chapter 15 (before/after comparison, `perf diff`, total-work
  normalization — effect size and confidence intervals applied
  specifically to proving an optimization).
- Chapter 17, 19 (parameter sweeps — where the multiple-comparisons
  caution above applies directly).
