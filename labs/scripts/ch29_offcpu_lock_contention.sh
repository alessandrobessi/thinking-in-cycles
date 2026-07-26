#!/usr/bin/env bash
#
# Chapter 29 guided lab: a real, tested on-CPU vs. off-CPU-inclusive
# comparison. macOS's `sample`(1) records every thread's stack on a
# wall-clock interval regardless of run state (blocked, waiting, or
# running) -- unlike Linux `perf record`'s on-CPU-only default. That
# makes it possible to see a genuinely blocked stack (a thread parked in
# a mutex wait) directly, on this book's own reference machine, without
# any Linux-only off-CPU tracing tool.
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CYCLELAB="$SCRIPT_DIR/../cyclelab/bin/cyclelab"
CAPTURE="$SCRIPT_DIR/capture_sample_profile.sh"
FOLD_TO_SVG="$SCRIPT_DIR/flamegraph_svg.py"

if [ ! -x "$CYCLELAB" ]; then
  echo "cyclelab binary not found at $CYCLELAB -- run 'make lab-cyclelab' from the repo root first." >&2
  exit 1
fi
if ! command -v sample >/dev/null 2>&1; then
  echo "'sample' not found -- this lab is macOS-specific." >&2
  echo "On Linux, use: perf record -F 99 -g -- \$CYCLELAB compute --duration=3" >&2
  echo "and an off-CPU-specific tool (e.g. offcputime-bpfcc) for the blocked side." >&2
  exit 1
fi

OUTDIR="${1:-/tmp/ch29_offcpu}"
mkdir -p "$OUTDIR"

echo "Chapter 29 lab: on-CPU-only workload vs. lock-contention (blocking) workload,"
echo "both captured with the same wall-clock sampler."
echo

echo "== compute mode (pure on-CPU, no blocking by design) =="
"$CAPTURE" "$OUTDIR/oncpu.folded" 2 -- compute --duration=3 --threads=4 --chains=1 >/dev/null
ONCPU_TOTAL=$(awk '{sum+=$NF} END{print sum+0}' "$OUTDIR/oncpu.folded")
ONCPU_BLOCKED=$(grep -E "psynch_mutexwait|mutex_firstfit_lock_wait" "$OUTDIR/oncpu.folded" | awk '{sum+=$NF} END{print sum+0}')
echo "  total sampled stack frames: $ONCPU_TOTAL"
echo "  frames inside a mutex wait: $ONCPU_BLOCKED"
echo

echo "== lock-contention mode (heavy contention, most threads should be blocked) =="
"$CAPTURE" "$OUTDIR/contention.folded" 2 -- lock-contention --duration=3 --threads=8 --hold-us=300 >/dev/null
CONT_TOTAL=$(awk '{sum+=$NF} END{print sum+0}' "$OUTDIR/contention.folded")
CONT_BLOCKED=$(grep -E "psynch_mutexwait|mutex_firstfit_lock_wait" "$OUTDIR/contention.folded" | awk '{sum+=$NF} END{print sum+0}')
echo "  total sampled stack frames: $CONT_TOTAL"
echo "  frames inside a mutex wait:  $CONT_BLOCKED"
echo

python3 -c "
oncpu_total, oncpu_blocked = $ONCPU_TOTAL, $ONCPU_BLOCKED
cont_total, cont_blocked = $CONT_TOTAL, $CONT_BLOCKED
oncpu_pct = 100.0 * oncpu_blocked / oncpu_total if oncpu_total else 0.0
cont_pct = 100.0 * cont_blocked / cont_total if cont_total else 0.0
print(f'compute mode:         {oncpu_pct:5.1f}% of sampled frames were inside a mutex wait')
print(f'lock-contention mode: {cont_pct:5.1f}% of sampled frames were inside a mutex wait')
"
echo

if [ -f "$FOLD_TO_SVG" ] && command -v python3 >/dev/null 2>&1; then
  python3 "$FOLD_TO_SVG" "$OUTDIR/contention.folded" -o "$OUTDIR/contention.svg" >/dev/null 2>&1 || true
  if [ -f "$OUTDIR/contention.svg" ]; then
    echo "Rendered: $OUTDIR/contention.svg -- frames containing psynch_mutexwait /"
    echo "mutex_firstfit_lock_wait in this flame graph ARE off-CPU time, captured"
    echo "directly, not inferred -- macOS's wall-clock sampler does not"
    echo "distinguish on-CPU from off-CPU the way a dedicated off-CPU tool would"
    echo "(it captures both without labeling which is which), so read width in"
    echo "this specific SVG as 'time in this state' only for frames you can"
    echo "identify as a wait path by name, not as a pure on-CPU cost breakdown."
  fi
fi
echo

echo "== context-switch counters for the same two workloads =="
CTX_ONCPU=$("$CYCLELAB" compute --duration=1 --threads=4 --chains=1 --quiet --format=text | grep context_switches)
CTX_CONT=$("$CYCLELAB" lock-contention --duration=1 --threads=8 --hold-us=300 --quiet --format=text | grep context_switches)
echo "  compute:         $CTX_ONCPU"
echo "  lock-contention: $CTX_CONT"
echo
echo "  Note: on this book's macOS reference machine, 'voluntary' reads 0 in"
echo "  every mode, including this one -- a real, confirmed platform"
echo "  limitation of Darwin's getrusage, not a sign lock-contention isn't"
echo "  really blocking (Step 2's mutex-wait stacks above prove it is). See"
echo "  labs/cyclelab/README.md's 'Context-switch reporting' section."
