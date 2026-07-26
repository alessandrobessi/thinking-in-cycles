#!/usr/bin/env bash
#
# Chapter 21 guided lab: sweep thread count well past this machine's core
# count and compare throughput against involuntary context switches --
# cyclelab's own process-wide getrusage(2) counters (see labs/cyclelab/README.md).
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
NCPU=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 8)
HALF=$((NCPU / 2)); [ "$HALF" -lt 1 ] && HALF=1
DOUBLE=$((NCPU * 2))
QUAD=$((NCPU * 4))

echo "Chapter 21 lab: throughput vs. involuntary context switches, thread count"
echo "sweeping from below to well above this machine's $NCPU logical CPUs."
echo

printf '%-8s %-18s %-14s\n' "threads" "throughput_ops_s" "nivcsw"
for THREADS in 1 "$HALF" "$NCPU" "$DOUBLE" "$QUAD"; do
  OUT="$("$CYCLELAB" compute --duration="$DURATION" --threads="$THREADS" --quiet)"
  printf '%s' "$OUT" | python3 -c "
import json, sys
d = json.load(sys.stdin)
r = d['results']
c = r['context_switches']
print(f\"{'$THREADS':<8} {r['throughput_ops_per_s']:<18.0f} {c['involuntary']:<14}\")
"
done

echo
echo "Interpretation: throughput should scale while threads <= $NCPU (this"
echo "machine's logical CPU count), then flatten once threads exceed it --"
echo "the CPUs are already fully occupied, so more threads cannot add more"
echo "completed work. Involuntary context switches, in contrast, should keep"
echo "climbing well past that point: each extra thread beyond $NCPU CPUs is"
echo "runnable but has to wait its turn, and every time the scheduler swaps"
echo "which thread gets to run, that is one more involuntary switch --"
echo "visible directly in this counter even though throughput alone would"
echo "make everything past $NCPU threads look the same."
