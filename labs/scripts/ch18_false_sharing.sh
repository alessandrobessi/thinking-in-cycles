#!/usr/bin/env bash
#
# Chapter 18 guided lab: compare packed (likely false-sharing) vs padded
# (one cache line per counter) throughput scaling across thread counts.
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

echo "Chapter 18 lab: packed vs. padded per-thread counters, throughput by thread count."
echo "(duration=$DURATION per run -- shorter durations can be noisy; Chapter 4's"
echo " variance caution applies here as much as anywhere else.)"
echo

printf '%-8s %-8s %-20s\n' "padding" "threads" "throughput_incr_s"
for PADDING in packed padded; do
  for THREADS in 1 2 4 8 10; do
    OUT="$("$CYCLELAB" false-sharing --padding="$PADDING" --threads="$THREADS" \
      --duration="$DURATION" --quiet)"
    TP=$(printf '%s' "$OUT" | python3 -c \
      "import json,sys; v=json.load(sys.stdin)['results']['throughput_increments_per_s']; print(f'{v:.0f}')")
    printf '%-8s %-8s %-20s\n' "$PADDING" "$THREADS" "$TP"
  done
done

echo
echo "Interpretation: at threads=1 there is nothing to share, so packed and"
echo "padded should be roughly equal. As thread count rises, expect padded"
echo "to increasingly outperform packed -- not because either version's"
echo "code does anything different (each thread only ever touches its own"
echo "counter), but because packed counters likely share a 64-byte cache"
echo "line, so every thread's write invalidates that line for every other"
echo "thread sharing it, forcing repeated cache-coherence traffic that"
echo "padded's one-counter-per-line layout cannot experience at all."
