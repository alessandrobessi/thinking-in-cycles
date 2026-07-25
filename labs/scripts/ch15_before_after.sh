#!/usr/bin/env bash
#
# Chapter 15 guided lab: prove an optimization with a controlled,
# interleaved benchmark (Chapter 4/5 discipline), then use before/after
# profiles -- including a differential flame graph -- to see how (or
# whether) the work's shape actually changed.
#
# "Baseline" is cyclelab compute --chains=1 (one dependency chain);
# "after" is --chains=8 (eight independent chains) -- the same real
# optimization Chapters 7-8 introduced, revisited here as a full
# investigation-loop exercise rather than a single measurement.
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CYCLELAB="$SCRIPT_DIR/../cyclelab/bin/cyclelab"

if [ ! -x "$CYCLELAB" ]; then
  echo "cyclelab binary not found at $CYCLELAB -- run 'make lab-cyclelab' from the repo root first." >&2
  exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 not found; needed to parse JSON and render the differential flame graph." >&2
  exit 1
fi

REPS="${1:-5}"
DURATION="${2:-0.5}"

echo "Chapter 15 lab, step 1: controlled, interleaved benchmark (Chapter 4 discipline)."
echo "baseline = --chains=1, after = --chains=8, $REPS repetitions each, alternating order."
echo

printf '%-6s %-10s %-18s\n' "rep" "config" "throughput_ops_s"
for ((r = 1; r <= REPS; r++)); do
  for CFG in baseline:1 after:8; do
    LABEL="${CFG%%:*}"; CHAINS="${CFG##*:}"
    OUT="$("$CYCLELAB" compute --duration="$DURATION" --threads=1 --op=int --chains="$CHAINS" --quiet)"
    T=$(printf '%s' "$OUT" | python3 -c \
      "import json,sys; v=json.load(sys.stdin)['results']['throughput_ops_per_s']; print(f'{v:.0f}')")
    printf '%-6s %-10s %-18s\n' "$r" "$LABEL" "$T"
  done
done

echo
echo "Chapter 15 lab, step 2: before/after profiles, equal SAMPLING DURATION"
echo "(not equal completed work -- see the interpretation note below)."
echo

if command -v sample >/dev/null 2>&1; then
  "$SCRIPT_DIR/capture_sample_profile.sh" /tmp/ch15_baseline.folded 2 -- \
    compute --duration=3 --threads=1 --op=int --chains=1 --quiet --output=/dev/null
  "$SCRIPT_DIR/capture_sample_profile.sh" /tmp/ch15_after.folded 2 -- \
    compute --duration=3 --threads=1 --op=int --chains=8 --quiet --output=/dev/null

  python3 "$SCRIPT_DIR/flamegraph_svg.py" /tmp/ch15_after.folded \
    --diff-against /tmp/ch15_baseline.folded -o /tmp/ch15_diff.svg

  echo
  echo "Differential flame graph written to /tmp/ch15_diff.svg"
  echo "(red = grew vs. baseline, blue = shrank, grey = ~unchanged)"
else
  echo "'sample' not found -- skipping the profiling half of this lab."
  echo "See the chapter text for the equivalent 'perf diff' commands on Linux."
fi

echo
echo "Interpretation: step 1 alone already proves whether the optimization"
echo "helped -- a real, controlled, interleaved throughput comparison,"
echo "exactly Chapter 4/5's discipline, needing no profiler at all. Step 2's"
echo "profiles explain HOW, but were captured for equal wall-clock sampling"
echo "duration, not equal completed work -- the 'after' config finishes far"
echo "more iterations in the same window (Chapter 7/8's throughput gap),"
echo "so its profile represents a different total amount of work than the"
echo "baseline's, despite sharing the same capture duration. Both profiles"
echo "should still show almost the same *shape* (compute_worker dominating"
echo "either way) -- which is itself the lesson: this particular"
echo "optimization changed how fast the code ran, not which code ran, so a"
echo "shape-based (sampling) view stays nearly flat while a counting-based"
echo "view (step 1's throughput) shows the real improvement clearly."
