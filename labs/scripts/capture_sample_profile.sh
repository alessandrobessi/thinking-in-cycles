#!/usr/bin/env bash
#
# Captures a real CPU profile of a cyclelab run using macOS's built-in
# `sample`(1) utility, folds it with foldstacks.py, and writes a
# .folded file usable by flamegraph_svg.py. This is the "capture" +
# "fold" half of the pipeline several Part III chapters share.
#
# Usage: capture_sample_profile.sh <output.folded> <sample-seconds> -- <cyclelab-args...>
# Example:
#   ./capture_sample_profile.sh /tmp/baseline.folded 2 -- compute --duration=3 --threads=1 --chains=1
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CYCLELAB="$SCRIPT_DIR/../cyclelab/bin/cyclelab"

if [ "$#" -lt 4 ] || [ "$3" != "--" ]; then
  echo "usage: $0 <output.folded> <sample-seconds> -- <cyclelab-args...>" >&2
  exit 64
fi
OUT_FOLDED="$1"
SAMPLE_SECONDS="$2"
shift 3

if [ ! -x "$CYCLELAB" ]; then
  echo "cyclelab binary not found at $CYCLELAB -- run 'make lab-cyclelab' from the repo root first." >&2
  exit 1
fi
if ! command -v sample >/dev/null 2>&1; then
  echo "'sample' not found. This capture script is macOS-specific" >&2
  echo "(hardware/OS-dependent). On Linux, use:" >&2
  echo "  perf record -F 99 -g -- $CYCLELAB $*" >&2
  echo "  perf script > out.perf && stackcollapse-perf.pl out.perf > out.folded" >&2
  exit 1
fi

"$CYCLELAB" "$@" &
CPID=$!
sleep 0.3
if ! kill -0 "$CPID" 2>/dev/null; then
  echo "cyclelab exited before sampling could start -- use a longer --duration." >&2
  exit 1
fi

RAW="$(mktemp)"
sample "$CPID" "$SAMPLE_SECONDS" -f "$RAW" >/dev/null 2>&1
wait "$CPID" 2>/dev/null

python3 "$SCRIPT_DIR/foldstacks.py" "$RAW" > "$OUT_FOLDED"
rm -f "$RAW"

LINES=$(wc -l < "$OUT_FOLDED" | tr -d ' ')
if [ "$LINES" -eq 0 ]; then
  echo "warning: captured zero call-path samples -- the workload may have finished before sampling began" >&2
  exit 1
fi
echo "wrote $OUT_FOLDED ($LINES unique call paths)" >&2
