#!/usr/bin/env bash
#
# Chapter 12 guided lab: capture a real CPU profile with macOS `sample`
# (this book's portable analog to `perf record` + `perf report` +
# `perf annotate`) and read self cost, inclusive cost, and source-line
# attribution directly from its output.
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CYCLELAB="$SCRIPT_DIR/../cyclelab/bin/cyclelab"

if [ ! -x "$CYCLELAB" ]; then
  echo "cyclelab binary not found at $CYCLELAB -- run 'make lab-cyclelab' from the repo root first." >&2
  exit 1
fi
if ! command -v sample >/dev/null 2>&1; then
  echo "'sample' not found -- this lab is macOS-specific. See the chapter text for the" >&2
  echo "equivalent 'perf record'/'perf report'/'perf annotate' commands on Linux." >&2
  exit 1
fi

DURATION="${1:-3}"
SAMPLE_SECONDS="${2:-2}"
RAW="/tmp/ch12_sample.txt"

echo "Chapter 12 lab: profiling cyclelab compute with macOS 'sample'."
echo

"$CYCLELAB" compute --duration="$DURATION" --threads=1 --chains=1 --quiet --output=/dev/null &
CPID=$!
sleep 0.3
sample "$CPID" "$SAMPLE_SECONDS" -f "$RAW" >/dev/null 2>&1
wait "$CPID" 2>/dev/null

echo "-- 'Sort by top of stack' (sample's own self-cost ranking) --"
awk '/^Sort by top of stack/{flag=1;next} /^$/{if(flag)exit} flag' "$RAW"

echo
echo "-- source-line attribution inside compute_worker (self cost, per line) --"
grep -E '^[[:space:]]*[0-9]+ compute_worker.*compute\.c:[0-9]+' "$RAW" | \
  sed -E 's/^[[:space:]]*([0-9]+) .*(compute\.c:[0-9]+)$/\2 \1/' | \
  awk '{ sum[$1] += $2 } END { for (line in sum) printf "  %-14s %d\n", line, sum[line] }' | \
  sort -k2 -rn

echo
echo "Interpretation: 'Sort by top of stack' is self cost -- where samples"
echo "landed at the innermost (leaf) frame, the direct equivalent of 'perf"
echo "report's default self-cost ranking. The per-line breakdown is what"
echo "'perf annotate' would show for a single hot function: not every line"
echo "inside compute_worker costs the same -- expect the count to concentrate"
echo "on a few lines (the accumulator update/branch), not spread evenly"
echo "across the whole function body."
echo
echo "Raw capture kept at $RAW for further inspection."
