#!/usr/bin/env bash
#
# Chapter 2 guided lab: run the same program at three input sizes and two
# operation mixes, to show that "faster" depends on the workload you pick.
# Requires python3 for JSON parsing (BLUEPRINT.md Section 7.7 explicitly
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

echo "Chapter 2 lab: same program, three input sizes, two operation mixes."
echo

printf '%-12s %-8s %-16s %-18s\n' "iterations" "op" "elapsed_s" "throughput_ops_s"
for ITER in 200000 5000000 100000000; do
  for OP in int mixed; do
    OUT="$("$CYCLELAB" compute --iterations="$ITER" --threads=1 --op="$OP" --quiet)"
    ELAPSED=$(printf '%s' "$OUT" | python3 -c \
      "import json,sys; d=json.load(sys.stdin); v=d['results']['duration_actual_s']; print(f'{v:.6f}')")
    THRPUT=$(printf '%s' "$OUT" | python3 -c \
      "import json,sys; d=json.load(sys.stdin); v=d['results']['throughput_ops_per_s']; print(f'{v:.0f}')")
    printf '%-12s %-8s %-16s %-18s\n' "$ITER" "$OP" "$ELAPSED" "$THRPUT"
  done
done

echo
echo "Interpretation: read this table by size, not by one 'is cyclelab fast'"
echo "number. Fixed per-run costs (thread start/join, warm-up) matter"
echo "proportionally more at small iteration counts than large ones, so the"
echo "relative gap between op=int and op=mixed does not have to stay constant"
echo "across sizes -- a change that looks negligible at the largest size can"
echo "dominate at the smallest one, or vice versa. That size-dependence is"
echo "the point: there is no single 'faster' without saying for what input."
