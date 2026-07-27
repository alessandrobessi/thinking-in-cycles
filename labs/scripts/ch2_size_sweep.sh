#!/usr/bin/env bash
#
# Chapter 2 guided lab: run the same program at three measurement lengths
# (iteration counts -- a proxy for N in T(N) = T_fixed + N*T_unit, not a
# changing input size) and two operation mixes, repeated several times
# per cell, to show that a single-run "X is faster" comparison becomes
# less trustworthy the shorter the measurement gets -- borrowing
# Chapter 4's own repetition discipline a couple of chapters early.
# Requires python3 for JSON parsing (this project's own rule explicitly
# allows "optional Python for result processing").
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

REPS="${1:-9}"

echo "Chapter 2 lab: same program, three measurement lengths, two operation"
echo "mixes, $REPS repetitions per cell (median and range reported)."
echo

printf '%-12s %-8s %-14s %-14s %-14s\n' "iterations" "op" "median_ops_s" "min_ops_s" "max_ops_s"
for ITER in 5000 5000000 100000000; do
  for OP in int mixed; do
    VALUES=()
    for ((r = 0; r < REPS; r++)); do
      OUT="$("$CYCLELAB" compute --iterations="$ITER" --threads=1 --op="$OP" --quiet)"
      T=$(printf '%s' "$OUT" | python3 -c \
        "import json,sys; print(json.load(sys.stdin)['results']['throughput_ops_per_s'])")
      VALUES+=("$T")
    done
    read -r MEDIAN MIN MAX <<< "$(printf '%s\n' "${VALUES[@]}" | python3 -c \
      "import sys,statistics as st; v=[float(x) for x in sys.stdin]; print(f'{st.median(v):.0f} {min(v):.0f} {max(v):.0f}')")"
    printf '%-12s %-8s %-14s %-14s %-14s\n' "$ITER" "$OP" "$MEDIAN" "$MIN" "$MAX"
  done
done

echo
echo "Interpretation: compare each cell's (max - min) spread across its own"
echo "$REPS repetitions to the gap between op=int's and op=mixed's medians at"
echo "the same iteration count. At the shortest length, the spread within a"
echo "single configuration is comparable to or larger than the gap between"
echo "configurations -- a single run cannot reliably rank them at all. At"
echo "longer lengths, medians settle into a more stable, smaller gap (int"
echo "consistently a little ahead of mixed, since mixed does strictly more"
echo "work per op -- see the chapter text's unit caveat), but even there,"
echo "trust the median across repeated runs, not any one run's number --"
echo "Chapter 4's discipline, needed here already."
