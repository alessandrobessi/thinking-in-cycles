#!/usr/bin/env bash
#
# Chapter 28 guided lab: bpftrace/BCC are not available on this book's
# macOS reference machine (Linux-only). This script builds the same
# *aggregation concepts* -- count grouped by key, and a histogram -- by
# hand, from cyclelab's own JSON output, entirely in user space. It is
# not a substitute for in-kernel aggregation's efficiency advantage
# (Chapter 27's Worked Example), but it is a real, portable way to
# practice the same "event, key, aggregate" thinking bpftrace scripts
# are built from.
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

echo "Chapter 28 lab: hand-rolled count-grouped-by and histogram aggregation"
echo "over cyclelab's own JSON output -- the same 'event, key, aggregate'"
echo "shape bpftrace/BCC scripts use, computed after the fact instead of"
echo "live inside the kernel."
echo

echo "== Step 1: 'list available probes' analog =="
echo "  Before writing any aggregation, see what this tool can even answer:"
echo "  $ $CYCLELAB --help   (enumerates every mode and option, cyclelab's"
echo "  own equivalent of 'bpftrace -l' listing attachable probes)"
echo

echo "== Step 2/3: count an event, grouped by key =="
echo "  bpftrace equivalent: @[tid] = count();"
echo "  Running lock-contention with 10 threads contending one mutex,"
echo "  then counting increments completed per thread (the 'key')."
echo
OUT="$("$CYCLELAB" lock-contention --duration=1.5 --threads=10 --hold-us=5 --quiet)"
printf '%s' "$OUT" | python3 -c "
import json, sys
d = json.load(sys.stdin)
threads = d['results']['threads']
print(f'{\"thread\":<10}{\"increments\":<12}')
for t in threads:
    print(f'{t[\"index\"]:<10}{t[\"increments\"]:<12}')
total = sum(t['increments'] for t in threads)
print(f'{\"TOTAL\":<10}{total:<12}')
"
echo

echo "== Step 4: measure a duration =="
echo "  bpftrace equivalent: @dur[tid] = nsecs - @start[tid];"
echo "  Per-thread elapsed_s from the same run (each thread ran until the"
echo "  shared 1.5s deadline, so this mainly shows scheduling variance,"
echo "  not work-done variance -- a real distinction a duration"
echo "  measurement forces you to notice)."
echo
printf '%s' "$OUT" | python3 -c "
import json, sys
d = json.load(sys.stdin)
threads = d['results']['threads']
for t in threads:
    print(f'  thread[{t[\"index\"]}] elapsed_s={t[\"elapsed_s\"]:.6f}')
"
echo

echo "== Step 5: build a histogram =="
echo "  bpftrace equivalent: @ = hist(value);"
echo "  Bucketing per-thread increments (Step 2's data) into power-of-2-"
echo "  style ranges, printed as an ASCII histogram."
echo
printf '%s' "$OUT" | python3 -c "
import json, sys
d = json.load(sys.stdin)
threads = d['results']['threads']
vals = [t['increments'] for t in threads]
lo, hi = min(vals), max(vals)
if hi == lo:
    hi = lo + 1
nbuckets = 5
width = (hi - lo) / nbuckets
buckets = [0] * nbuckets
for v in vals:
    idx = min(int((v - lo) / width), nbuckets - 1)
    buckets[idx] += 1
for i, count in enumerate(buckets):
    lo_b = lo + i * width
    hi_b = lo + (i + 1) * width
    bar = '#' * count
    print(f'  [{lo_b:8.0f}, {hi_b:8.0f}) {bar} ({count})')
"
echo

echo "== Step 6: capture stacks (already built, Part III's pipeline) =="
echo "  bpftrace equivalent: @[kstack, ustack] = count();"
echo "  This book already has a real, tested stack-capture pipeline --"
echo "  capture_sample_profile.sh + foldstacks.py -- built for Chapters"
echo "  11-15 and reused directly in Chapter 29's off-CPU lab. It is not"
echo "  rebuilt here."
echo

echo "== Step 7: prefer a packaged tool before a custom script =="
echo "  Chapter 21's ch21_runnable_pressure.sh already IS a packaged tool"
echo "  for exactly one common question (throughput vs. involuntary"
echo "  switches across a thread-count sweep) -- reach for an existing"
echo "  script like that before hand-rolling a new aggregation, the same"
echo "  policy Chapter 28's own tool-choice guidance recommends for"
echo "  packaged BCC tools on real Linux hardware."
