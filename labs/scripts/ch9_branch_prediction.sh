#!/usr/bin/env bash
#
# Chapter 9 guided lab: cyclelab branch with predictable (sorted) and
# unpredictable (random) data through the exact same conditional.
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

DURATION="${1:-1}"
TABLE_SIZE="${2:-2000000}"

echo "Chapter 9 lab: cyclelab branch, sorted vs. random data through the same conditional."
echo "(table size: $TABLE_SIZE elements per thread)"
echo

printf '%-10s %-22s\n' "pattern" "throughput_elements_s"
for PATTERN in sorted random; do
  OUT="$("$CYCLELAB" branch --duration="$DURATION" --threads=1 --pattern="$PATTERN" \
    --branch-table-size="$TABLE_SIZE" --quiet)"
  T=$(printf '%s' "$OUT" | python3 -c \
    "import json,sys; v=json.load(sys.stdin)['results']['throughput_elements_per_s']; print(f'{v:.0f}')")
  printf '%-10s %-22s\n' "$PATTERN" "$T"
done

echo
echo "Interpretation: 'sorted' should show substantially higher throughput"
echo "than 'random', even though both walk the same number of elements"
echo "through the exact same conditional (if value >= 128 ... else ...)."
echo "The only difference is how easy the sequence of branch outcomes is to"
echo "predict: sorted data produces long runs of the same outcome; random"
echo "data flips unpredictably close to every element. Do not expect a"
echo "specific ratio -- expect 'sorted' to win, often by a wide margin."
