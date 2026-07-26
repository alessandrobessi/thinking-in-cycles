#!/usr/bin/env bash
#
# Chapter 16 guided lab: sweep working-set size with cyclelab's
# random-memory (pointer-chase) mode to find the latency cliffs where
# one level of the cache hierarchy stops being big enough.
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

echo "Chapter 16 lab: pointer-chase latency vs. working-set size (random-memory mode)."
echo

printf '%-10s %-16s\n' "size" "ns_per_access"
for SIZE in 16K 32K 64K 128K 256K 512K 1M 2M 4M 8M 16M 32M 64M 128M; do
  OUT="$("$CYCLELAB" random-memory --working-set-size="$SIZE" --duration="$DURATION" --threads=1 --quiet)"
  NS=$(printf '%s' "$OUT" | python3 -c \
    "import json,sys; v=json.load(sys.stdin)['results']['ns_per_access']; print(f'{v:.3f}')")
  printf '%-10s %-16s\n' "$SIZE" "$NS"
done

echo
echo "Interpretation: expect ns_per_access to stay roughly flat within one cache"
echo "level's capacity, then jump sharply once the working set outgrows it --"
echo "not a smooth curve, but a staircase with a small number of distinct"
echo "steps (typically L1, L2, and last-level-cache-to-DRAM). The exact sizes"
echo "and step heights are specific to this machine's cache hierarchy; do not"
echo "expect the same numbers on a different CPU. Compare against"
echo "'sequential-memory' at the same sizes (ch17's lab) to see how much of"
echo "this cost the hardware prefetcher can hide when access is predictable."
