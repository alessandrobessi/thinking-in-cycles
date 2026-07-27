#!/usr/bin/env bash
#
# Chapter 8 guided lab: sweep --chains from 1 to 16 for the same
# --op=int instruction mix, several repetitions per chain count
# (median reported), to see the pipeline reward independent work up to
# a point, then behave unevenly rather than flatten cleanly.
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
echo

printf '%-8s %-20s\n' "chains" "median_ops_s"
for CHAINS in 1 2 4 6 8 10 12 14 16; do
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
echo "then -- rather than settling into a clean plateau -- become noticeably"
echo "uneven: some chain counts clearly outperform their neighbors, in a"
echo "pattern that's reproducible on a given machine/compiler but not"
echo "predictable from chain count alone. On this book's reference machine,"
echo "chain counts that divide evenly into the hardware's vector width (4, 8,"
echo "16) consistently outperform the ones that don't (6, 10, 12, 14) -- a"
echo "second mechanism (how well the compiler can fit that many accumulators"
echo "into physical registers/vector units for that specific chain count) on"
echo "top of the first one (the pipeline rewarding independent work up to its"
echo "own issue-width limit). Do not expect the same favored chain counts on"
echo "a different machine or compiler; do expect the initial 1-to-4 rise and"
echo "some uneven, non-monotonic shape past it, rather than a flat ceiling."
