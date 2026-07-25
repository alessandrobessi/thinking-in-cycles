#!/usr/bin/env bash
#
# Chapter 8 guided lab: sweep --chains from 1 to 16 for the same
# --op=int instruction mix, to see the pipeline reward independent work
# up to a point, then stop rewarding it.
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

echo "Chapter 8 lab: throughput vs. independent-chain count (--chains), same instruction mix throughout."
echo

printf '%-8s %-20s\n' "chains" "throughput_ops_s"
for CHAINS in 1 2 4 8 12 16; do
  OUT="$("$CYCLELAB" compute --duration="$DURATION" --threads=1 --op=int --chains="$CHAINS" --quiet)"
  T=$(printf '%s' "$OUT" | python3 -c \
    "import json,sys; v=json.load(sys.stdin)['results']['throughput_ops_per_s']; print(f'{v:.0f}')")
  printf '%-8s %-20s\n' "$CHAINS" "$T"
done

echo
echo "Interpretation: throughput should rise as --chains goes from 1 toward"
echo "somewhere in the middle of this range, then flatten (and can even dip"
echo "slightly) rather than keep rising all the way to 16. The flattening"
echo "point is roughly where this CPU's available instruction-level"
echo "parallelism for this instruction mix is exhausted -- more independent"
echo "chains than that stop helping because there's no more room in the"
echo "pipeline to run them concurrently. A single run can be noisy near that"
echo "plateau (Chapter 4's variance caution applies here too); focus on the"
echo "overall rise-then-flatten shape, not the exact chains value where it happens."
