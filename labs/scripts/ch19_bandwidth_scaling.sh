#!/usr/bin/env bash
#
# Chapter 19 guided lab: sweep thread count with cyclelab's bandwidth
# mode (a large, prefetch-friendly streaming buffer per thread) to find
# where sustained throughput stops scaling -- the memory channels
# saturating, not the CPUs running out of work to issue.
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

DURATION="${1:-0.5}"
NCPU=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 8)

echo "Chapter 19 lab: aggregate streaming bandwidth vs. thread count."
echo "(this machine reports $NCPU logical CPUs; working set 64M per thread,"
echo " comfortably larger than any per-core cache)"
echo

printf '%-8s %-16s\n' "threads" "bandwidth_gb_s"
for THREADS in 1 2 4 6 8 "$NCPU"; do
  OUT="$("$CYCLELAB" bandwidth --working-set-size=64M --threads="$THREADS" \
    --duration="$DURATION" --quiet)"
  BW=$(printf '%s' "$OUT" | python3 -c \
    "import json,sys; v=json.load(sys.stdin)['results']['bandwidth_gb_per_s']; print(f'{v:.2f}')")
  printf '%-8s %-16s\n' "$THREADS" "$BW"
done

echo
echo "Interpretation: expect bandwidth to rise close to linearly at first"
echo "(2 threads roughly 2x 1 thread's number), then flatten well before"
echo "reaching this machine's full core count -- the point past which more"
echo "threads asking for more data cannot make the memory system deliver it"
echo "faster, because the shared channels between cores and DRAM are"
echo "already close to their sustainable rate. Compare the single-thread"
echo "number here against 'random-memory' mode's implied bandwidth"
echo "(64 bytes / ns_per_access) at the same working set: pointer-chasing"
echo "achieves a small fraction of this mode's throughput despite touching"
echo "memory just as continuously -- the latency-bound vs. bandwidth-bound"
echo "distinction this chapter is about."
