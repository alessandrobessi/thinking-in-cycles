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
chains=1: 655.0M 679.5M 680.6M 686.8M 691.7M 690.3M 697.2M 693.8M 694.5M 693.4M 696.0M 697.5M
chains=2: 1298.2M 1299.4M 1299.6M 1297.7M 1298.2M 1295.5M 1296.1M 1294.0M 1297.8M 1292.2M 1294.4M 1294.9M
```

**No real effect** (`--chains=1` run twice, as two separate sessions):

```text
run 1: 708.1M 707.2M 713.9M 705.0M 714.2M 710.9M 712.3M 712.0M 704.4M 709.9M 715.0M 711.9M
run 2: 711.8M 713.8M 713.7M 712.6M 715.1M 711.1M 711.0M 712.2M 714.2M 711.9M 714.2M 715.2M
```

## Medians and percentiles

Chapter 4 already prefers the median and relevant percentiles over the
mean, for the same reason this appendix's own data reinforces: a mean
is more sensitive to a single outlier run (a thermal event, a
background process, one unlucky scheduling decision) than a median is.
The `chains=1` data above has one visible low outlier (655.0M against a
cluster around 680-697.5M) — a mean would shift noticeably toward it;
the median (692.6M) barely moves. Percentiles generalize this:
reporting p50/p90 of a benchmark's own repeated runs (not just the
workload's own latency distribution, Chapter 3's tail latency) shows
whether a "typical" run and a "worse" run are close together or far
apart — informative on its own, before even comparing two
configurations. One honest limit worth stating explicitly: with twelve
repetitions, a p99 is not a meaningful tail estimate — p99 of twelve
values is essentially just the maximum, and the maximum of a small
sample is a noisy, high-variance statistic in its own right, not a
stable characterization of "how bad the worst case gets." Reaching for
p99 specifically requires far more repetitions than this appendix uses
(hundreds, not a dozen); with a dozen runs, report the median, and p90
only loosely, rather than reading precision into a number this small a
sample can't actually support.

## Confidence intervals

A **confidence interval** answers a narrower question than it's often
given credit for: not "where is the true value," but "given this
sample, what range of values would be consistent with having produced
it." A **bootstrap** confidence interval builds this range empirically
— resampling the observed data with replacement, many times, and taking
the middle 95% of the resulting distribution of a statistic — rather
than assuming a specific theoretical distribution shape.

The statistic worth bootstrapping directly is the *difference* (or
ratio) between the two configurations, not each configuration's own
mean separately: checking whether two marginal confidence intervals
happen to overlap is a real but conservative, less sensitive test than
computing a confidence interval for the difference itself, and the two
can disagree — two overlapping marginal intervals can still hide a real,
detectable difference between them. Applied to this appendix's data
(2,000 resamples, resampling each configuration's twelve values and
recomputing the mean difference each time, 95% interval): the real
effect gives a difference of [602.8M, 616.1M] ops/s (`chains=2` minus
`chains=1`) and a ratio of [1.87x, 1.91x] — entirely positive, entirely
above 1x, real evidence the two configurations differ, consistent with
the separate-interval comparison below but more direct.

The same procedure applied to the two `chains=1` *sessions* — the same
configuration, run in two separate back-to-back sessions — gives a
difference of [0.5M, 4.8M] ops/s and a ratio of [1.0007x, 1.0067x]:
barely, but detectably, distinguishable from zero *for these two
specific sessions*. That last qualifier matters: this is one session
compared against one other session, twelve repetitions each, not many
independently-collected session pairs — the bootstrap resamples
individual runs as though they were independent draws from each
session's own distribution, which they reasonably are *within* a
session, but it says nothing about whether this particular gap would
show up again between a fresh pair of sessions, since there is only one
pair here to begin with. The honest reading is narrower than "the two
sessions really differ, reproducibly": it's "these two specific
sessions were detectably different from each other, by something —
thermal state, frequency scaling, background load, or genuine
run-to-run drift are all plausible, and nothing in this data
distinguishes between them." That is itself the useful lesson: even a
"same configuration, run twice" comparison isn't immune to whatever
changed between the two sessions, which is exactly why Chapter 4 and
Chapter 8's own guided lab interleave configurations *within* one
session rather than trusting two separate blocks run back to back.
Comparing the two configurations' separate marginal intervals ([708.4M,
712.3M] and [712.3M, 713.9M], meeting almost exactly at their shared
edge) would have looked like clean "no difference" and missed this
smaller gap entirely — a real illustration of why the direct-difference
method is more sensitive, whatever the correct explanation for what it
found here turns out to be.

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

A **p-value**, or — as the Confidence Intervals section above worked
through directly — a confidence interval for the *difference* that
excludes zero (or for the *ratio* that excludes 1x), answers "is there
probably a real difference"; **effect size** answers the separate
question "how big is it, in a unit that doesn't depend on sample size."
Cohen's *d* (the difference between two means, divided by their pooled
standard deviation) applied to this appendix's real-effect comparison
gives *d* ≈ 70 — an extreme effect by any conventional threshold
(0.2 small, 0.5 medium, 0.8 large), and *d* this large is exactly what
low run-to-run variance plus a genuinely large mean shift produces: the
pooled standard deviation here (about 8.6M) is tiny relative to the
608.5M-unit gap between the means, so the same gap that shows up as
`--chains=2` running roughly 1.88x the throughput of `--chains=1` also
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
even with zero real effects present anywhere. Chapter 17's stride sweep
and Chapter 8's chain-count sweep are exactly this situation in
miniature — many configurations measured in one investigation — but the
protection isn't "look for a smooth, monotonic trend and distrust
anything else." Both of those chapters' own real, repeatedly-confirmed
results are *not* monotonic (Chapter 17's stride-vs-latency curve dips
and rises across the stride range; Chapter 8's chains-vs-throughput
curve favors specific chain counts and dips at others), and both are
genuine hardware behavior, not noise — a monotonicity filter would have
thrown out real findings in exactly the two chapters that most needed
a multiple-comparisons discipline. Repeating each configuration (not
just each pairwise comparison) enough times to know its own spread is
necessary — a result that isn't stable across repeats was never
trustworthy regardless of how many configurations were tested — but
it is not, by itself, sufficient: reproducibility confirms a specific
measurement is real and stable, it does not control the probability
that, somewhere across many honestly-measured, individually-reproducible
configurations, one or two look unusually large or small purely by
chance. The actual protection against that is holding a sweep's own
standout result to a higher bar than a single configuration measured on
its own: treat anything that looks surprising *within* a sweep as a
hypothesis the sweep generated, not a conclusion it proved, and confirm
it with an independent run collected afterward, specifically targeting
that one configuration — the same discipline as any single real-effect
comparison in this appendix, just applied a second time before trusting
whichever point in the sweep looked most interesting.

## Practical versus statistical significance

The distinction this whole appendix builds toward: a difference can be
statistically well-established within the data at hand (a difference
confidence interval that excludes zero) and still not matter for any
decision that depends on it — and, separately, "well-established in
this data" is not automatically "would show up again." This appendix's
own "no real effect" pair makes both points at once: the two `chains=1`
sessions differ detectably, by roughly 0.07-0.67%, within these two
specific sessions — genuinely "significant" by the direct-difference
test, and small enough that it wouldn't be worth chasing even if it
were a real, per-run effect. But with only one session on each side,
this appendix's own data cannot say whether that gap would recur
between a fresh pair of sessions, or whether it was specific to
whatever changed between these two particular ones. Both readings point
the same direction in practice — don't chase it — but for different
reasons, and only the weaker one ("too small to matter, even if real")
is something this specific comparison actually established; the
stronger one ("a real, reproducible per-run effect") would need
multiple independent session pairs, not the twelve-repetitions-times-two
this appendix has. The reverse also holds: a difference too small to
distinguish from noise in ten runs might still be real and meaningful,
just under-measured — more repetitions, not a different conclusion, is
the correct response to that specific situation.
Chapter 4's own discipline (state what the data supports and what it
doesn't, separate observation from recommendation) is this appendix's
statistical vocabulary applied to two sentences instead of a formula.

## Related

- Chapter 4 (repetition, distribution, outliers — the qualitative
  version of this appendix's vocabulary).
- Chapter 15 (before/after comparison, `perf diff`, total-work
  normalization — effect size and confidence intervals applied
  specifically to proving an optimization).
- Chapter 8, 17, 19 (parameter sweeps — where the multiple-comparisons
  caution above applies directly, and where Chapter 8's and Chapter
  17's own non-monotonic-but-real results are the reason that caution
  isn't "distrust anything that isn't a smooth trend").
