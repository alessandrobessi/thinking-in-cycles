#!/usr/bin/env bash
#
# Chapter 4 guided lab: run two nearly-identical configurations
# (op=int vs op=float) interleaved rather than blocked, and print every
# repetition instead of just an average, so the reader sees a distribution.
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

REPS="${1:-8}"
DURATION="${2:-0.5}"

echo "Chapter 4 lab: interleaved A/B comparison of op=int vs op=float,"
echo "$REPS repetitions each, alternating order (not run back-to-back)."
echo

printf '%-6s %-8s %-18s\n' "rep" "op" "throughput_ops_s"
for ((r = 1; r <= REPS; r++)); do
  for OP in int float; do
    OUT="$("$CYCLELAB" compute --duration="$DURATION" --threads=1 --op="$OP" --quiet)"
    THRPUT=$(printf '%s' "$OUT" | python3 -c \
      "import json,sys; v=json.load(sys.stdin)['results']['throughput_ops_per_s']; print(f'{v:.0f}')")
    printf '%-6s %-8s %-18s\n' "$r" "$OP" "$THRPUT"
  done
done

echo
echo "Interpretation: look at the *spread* within each column, not just its"
echo "average. If the int and float columns overlap as much as each column"
echo "varies against itself, a single run's difference between them is not"
echo "trustworthy evidence (Misconception M15). See"
echo "labs/expected-shapes/ch04-distribution-shape.md for what to expect."
