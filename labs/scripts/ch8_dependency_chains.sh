#!/usr/bin/env bash
#
# Chapter 8 guided lab: sweep --chains over 1, 2, 4, 8, 16 for the same
# --op=int instruction mix, several repetitions per chain count (median
# reported), to see the pipeline reward independent work up to a point,
# then behave unevenly rather than flatten cleanly.
#
# Only chain counts that divide 16 evenly are tested: cyclelab's compute
# mode unrolls exactly 16 slots per iteration (Chapter 7), so a chain
# count that doesn't divide 16 evenly (6, 10, 12, 14, ...) hands some
# chains one more update than others within the same iteration -- a
# longer critical path for those chains that has nothing to do with the
# hardware being measured. Testing only divisors of 16 removes that
# confound by construction instead of needing to explain it away.
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CYCLELAB="$SCRIPT_DIR/../cyclelab/bin/cyclelab"

if [ ! -x "$CYCLELAB" ]; then
  echo "cyclelab binary not found at $CYCLELAB -- run 'make lab-cyclelab' from the repo root first." >&2
  exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 not found; this script needs it to parse cyclelab's JSON output." >&2
  exit 1
fi

DURATION="${1:-0.5}"
REPS="${2:-9}"

echo "Chapter 8 lab: throughput vs. independent-chain count (--chains), same"
echo "instruction mix throughout, $REPS repetitions per chain count (median shown)."
echo "Only chain counts dividing 16 evenly are tested (1, 2, 4, 8, 16) -- see"
echo "this script's header comment for why."
echo

printf '%-8s %-20s\n' "chains" "median_ops_s"
for CHAINS in 1 2 4 8 16; do
  VALUES=()
  for ((r = 0; r < REPS; r++)); do
    OUT="$("$CYCLELAB" compute --duration="$DURATION" --threads=1 --op=int --chains="$CHAINS" --quiet)"
    T=$(printf '%s' "$OUT" | python3 -c \
      "import json,sys; print(json.load(sys.stdin)['results']['throughput_ops_per_s'])")
    VALUES+=("$T")
  done
  MEDIAN=$(printf '%s\n' "${VALUES[@]}" | python3 -c \
    "import sys,statistics as st; print(f'{st.median([float(x) for x in sys.stdin]):.0f}')")
  printf '%-8s %-20s\n' "$CHAINS" "$MEDIAN"
done

echo
echo "Interpretation: throughput should rise sharply from 1 chain toward 4,"
echo "then -- rather than settling into a clean plateau -- stay uneven even"
echo "across this fairness-controlled set (every tested value divides 16"
echo "evenly, so no chain count here gets an unfair, longer critical path"
echo "than another). On this book's reference machine, 8 chains measures a"
echo "little below 4, and 16 is the fastest of all. That remaining"
echo "unevenness is real and reproducible, but this black-box throughput"
echo "sweep cannot say why -- register allocation, code layout, and other"
echo "compiler-specific choices are all plausible candidates, and pinning"
echo "one down requires reading the generated assembly (cc -S) for the"
echo "specific chain counts in question, not just comparing throughput"
echo "numbers. Do not expect the same favored chain counts on a different"
echo "machine or compiler; do expect the initial 1-to-4 rise and some"
echo "uneven, non-monotonic shape past it, rather than a flat ceiling."
