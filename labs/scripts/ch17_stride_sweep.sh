#!/usr/bin/env bash
#
# Chapter 17 guided lab: sweep --stride at a fixed, larger-than-cache
# working set (sequential-memory mode) to see how access order alone --
# not working-set size, held constant here -- changes latency. A stride
# of N slots (each CYCLELAB_CACHE_LINE_BYTES=64 bytes) is the same shape
# as row-major (stride=1) vs. column-major (stride=row length) traversal
# of a 2D array.
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

DURATION="${1:-0.3}"
# 64000192 = 1000003 (prime) * 64 -- a prime slot count so that no
# stride below accidentally visits only a short sub-cycle of the buffer
# (a power-of-2 working set with a power-of-2 stride would do exactly
# that, confounding "stride" with "effective working set size").
WORKING_SET=64000192

echo "Chapter 17 lab: stride vs. latency at a fixed 64MB (>> cache) working set."
echo

printf '%-8s %-16s\n' "stride" "ns_per_access"
for STRIDE in 1 2 4 8 16 32 64 128 256 512 1024; do
  OUT="$("$CYCLELAB" sequential-memory --working-set-size=$WORKING_SET --stride="$STRIDE" \
    --duration="$DURATION" --threads=1 --quiet)"
  NS=$(printf '%s' "$OUT" | python3 -c \
    "import json,sys; v=json.load(sys.stdin)['results']['ns_per_access']; print(f'{v:.3f}')")
  printf '%-8s %-16s\n' "$STRIDE" "$NS"
done

echo
echo "Interpretation: stride=1 (fully contiguous -- 'row-major') should be"
echo "dramatically faster than every other stride tested -- the one pattern"
echo "every hardware prefetcher is built to recognize. Do NOT expect a smooth,"
echo "monotonic curve for stride>1: real prefetchers have specific stride"
echo "patterns they detect well or poorly, so the middle of this table can"
echo "be noisy and non-monotonic rather than a clean 'bigger stride = worse'"
echo "line -- that irregularity is real hardware behavior, not measurement"
echo "error, and it is itself the lesson: 'predictable access' means"
echo "matching what this specific hardware's prefetcher can exploit, not"
echo "just 'small stride is always better than large stride.'"
