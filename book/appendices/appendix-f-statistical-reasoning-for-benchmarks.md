# Appendix F — Statistical Reasoning for Benchmarks

**Status:** reference material, not a chapter. Builds directly on
Chapter 4's benchmarking-hygiene material and Chapter 15's
before/after discipline, with the specific statistical vocabulary
those chapters use informally. Every number below is real, generated
for this appendix on this book's own reference machine — not
illustrative placeholders.

## The real data this appendix uses

Two comparisons, twelve *paired blocks* each, `cyclelab compute
--duration=0.3 --threads=1 --op=int`. Each block runs both
configurations back to back, in a freshly randomized order within the
block (not all of one configuration's runs, then all of the other's,
in two separate sessions) — the same interleaving discipline Chapter
4/15 use for a single before/after comparison and Chapter 8 uses for
its five-way chain-count sweep, applied here to keep this appendix's
own statistics honest about what they can and can't establish:

**A real effect** (`--chains=1` vs. `--chains=2`, paired by block):

```text
block     1      2      3      4      5      6      7      8      9     10     11     12
chains=1  725.4  720.8  726.0  671.8  719.2  722.7  723.6  721.3  713.5  716.5  718.9  716.2
chains=2 1356.3 1338.7 1314.4 1329.9 1328.3 1332.6 1327.8 1327.7 1318.9 1319.3 1300.0 1313.0
```

**No real effect** (`--chains=1` run twice per block, order randomized
within each block — not two separate sessions):

```text
block    1      2      3      4      5      6      7      8      9     10     11     12
run A   719.2  717.0  712.6  719.7  717.8  710.6  702.9  713.3  713.1  715.5  717.4  714.0
run B   719.6  708.1  721.4  721.0  713.9  715.8  700.6  718.4  712.0  717.9  716.5  720.1
```

(Both tables in millions of ops/s.)

## Medians and percentiles

Chapter 4 already prefers the median and relevant percentiles over the
mean, for the same reason this appendix's own data reinforces: a mean
is more sensitive to a single outlier run (a thermal event, a
background process, one unlucky scheduling decision) than a median is.
The `chains=1` column above has one visible low outlier (671.8M in
block 4, against a cluster around 713-726M) — a mean would shift
noticeably toward it; the median (720.0M) barely moves. Percentiles
generalize this: reporting p50/p90 of a benchmark's own repeated runs
(not just the workload's own latency distribution, Chapter 3's tail
latency) shows whether a "typical" run and a "worse" run are close
together or far apart — informative on its own, before even comparing
two configurations. One honest limit worth stating explicitly: with
twelve repetitions, a p99 is not a meaningful tail estimate — p99 of
twelve values is essentially just the maximum, and the maximum of a
small sample is a noisy, high-variance statistic in its own right, not
a stable characterization of "how bad the worst case gets." Reaching
for p99 specifically requires far more repetitions than this appendix
uses (hundreds, not a dozen); with a dozen runs, report the median, and
p90 only loosely, rather than reading precision into a number this
small a sample can't actually support.

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
mean separately, and — since this appendix's data is paired by block —
the resampling unit should be the *block*, not the individual run:
resample the twelve blocks with replacement, and for each resampled set
recompute the mean of that block's own paired difference (`chains=2`'s
value minus `chains=1`'s value, within the same block). Resampling
individual runs instead of blocks would implicitly assume every run is
an independent draw regardless of which block it came from — an
assumption this appendix's own paired collection doesn't need to make,
since pairing is exactly what controls for whatever varies block to
block (thermal state, frequency scaling, background load) without
having to assume it away.

Applied to this appendix's data (2,000 resamples of the twelve blocks,
95% interval): the real effect gives a block-level difference of
[599.1M, 621.4M] ops/s and a ratio of [1.83x, 1.88x] — entirely
positive, entirely above 1x, strong evidence the two configurations
differ *within this specific paired-block experiment*. A 95% interval
is itself a probabilistic statement about the procedure, not a
guarantee about this one result — under repeated experiments following
this same method, about 95% of the resulting intervals should cover the
true difference, which means an individual interval, however wide it
falls from zero, is evidence for this experiment, not automatic proof
that a fresh, independently-collected confirmation run would agree.
That confirmation run is a genuinely separate step this appendix
doesn't take.

The same procedure applied to the "no real effect" pair — the same
configuration, twice per block, order randomized within each block —
gives a block-level difference of [-1.6M, 3.5M] ops/s and a ratio of
[0.998x, 1.005x]: an interval that **includes zero** (equivalently,
includes 1x for the ratio), so this experiment does not distinguish the
two nominally identical runs — the outcome a correctly-controlled null
comparison ought to produce, and notably different from what an
*un*paired comparison produced earlier in this appendix's own drafting.
Resampling *individual* runs from two separately-collected, un-paired
sessions instead (as an earlier draft of this appendix did) produced an
interval that barely excluded zero, despite nothing about the underlying
configurations differing — not because the direct-difference method is
wrong, but because treating two un-paired sessions' runs as freely
interchangeable throws away the information that would have correctly
attributed a session-to-session shift to the sessions, not to the
configuration. Pairing by block, and resampling at the block level, is
what let this particular experiment's null result come out looking
null; a different null comparison, paired or not, could still land
differently by chance, which is exactly why a single interval —
including this one — is evidence, not proof.

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

Because this appendix's data is paired by block, the effect size worth
computing is the *paired* Cohen's *dz* — the mean of the twelve
block-level differences, divided by the standard deviation of those
same twelve differences — not the unpaired *d* (the difference between
two marginal means, divided by their pooled marginal standard
deviation), which throws away the pairing the rest of this appendix's
analysis deliberately keeps. Applied to this appendix's real-effect
comparison: the twelve block differences have mean 609.3M ops/s and
standard deviation 20.0M ops/s, giving *dz* ≈ 30 — an extreme effect by
any conventional threshold (0.2 small, 0.5 medium, 0.8 large), and *dz*
this large is exactly what low block-to-block variance in the
*difference itself* plus a genuinely large mean shift produces: the
609.3M-unit average gap is over thirty times the spread of that gap
across blocks, so the same effect that shows up as `--chains=2` running
roughly 1.85x the throughput of `--chains=1` also shows up as a *dz* far
outside the range effect-size guidance is usually written for — a
reminder that Cohen's thresholds were calibrated on noisier measurements
than a tight, low-variance, paired benchmark comparison typically
produces. For performance work specifically, the throughput ratio
(1.85x, with its own paired-block confidence interval already computed
above) is usually the more directly useful number to report alongside
*dz* — it answers "how much faster," which is what most engineering
decisions actually need. *dz* answers a narrower, different question:
"how large and consistent is the per-block gap, relative to how much
that gap itself varies from block to block" — a statement about the
*paired differences*, not about how far apart the two configurations'
raw throughput distributions sit from each other (that's what the
unpaired *d* would describe, and exactly why this appendix uses the
paired version instead, per its own paired collection design). The
reason effect size matters separately from "is it real":
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
anything else." Both of those chapters' own results, repeatedly
observed within their respective exploratory sweeps, are *not*
monotonic (Chapter 17's stride-vs-latency curve dips and rises across
the stride range; Chapter 8's chains-vs-throughput curve favors
specific chain counts and dips at others) — Chapter 8 itself is
explicit that it cannot attribute its own pattern to hardware, compiler
code generation, or some combination without reading the generated
assembly, and neither chapter's own sweep is presented as an
independently-confirmed finding in the stronger sense this appendix's
own "Repeated comparisons" discipline calls for below; both are
observations a monotonicity filter would have thrown out regardless,
which is the only point that matters here: whatever their ultimate
cause, and whatever further confirmation either pattern would still
need, discarding a non-monotonic result on shape alone would have lost
real signal in exactly the two chapters that most needed a
multiple-comparisons discipline. Repeating each configuration (not
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
statistically well-established within a single well-controlled
experiment (a block-level difference confidence interval that excludes
zero) and still not matter for any decision that depends on it — a
real, repeatable 0.3% throughput difference would be "significant" in
the statistical sense demonstrated above, but rarely worth the
engineering cost of chasing unless the specific context makes 0.3%
valuable at scale. This appendix's actual real-effect comparison is
nowhere near that marginal — `chains=2`'s 1.83x-1.88x block-level
speedup is both statistically well-established *and* large enough to
matter for essentially any purpose — but the
distinction still applies in general, and shows up concretely in the
"no real effect" pair: once properly paired and interleaved, its
block-level difference interval includes zero, correctly signaling
nothing here is worth chasing at all, statistically or practically.
The reverse also holds: a difference too small to distinguish from
noise in ten runs might still be real and meaningful, just
under-measured — more repetitions, not a different conclusion, is
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
