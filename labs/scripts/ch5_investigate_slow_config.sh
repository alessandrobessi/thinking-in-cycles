#!/usr/bin/env bash
#
# Chapter 5 guided lab: run a deliberately over-threaded cyclelab compute
# configuration and walk through the investigation loop (Section 5) using
# only wall time, CPU time, process state, and basic system counters.
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

NCPU=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
OVER_THREADS=$((NCPU * 4))

print_result() {
  python3 -c '
import json, sys
d = json.load(sys.stdin)
r = d["results"]
duration = r["duration_actual_s"]
iterations = r["total_iterations"]
throughput = r["throughput_ops_per_s"]
print(f"  duration_actual_s: {duration:.3f}")
print(f"  total_iterations:  {iterations}")
print(f"  throughput_ops_s:  {throughput:.0f}")
'
}

echo "Chapter 5 lab: a deliberately misconfigured run to investigate."
echo "This machine reports $NCPU logical CPUs; the first run below asks for"
echo "$OVER_THREADS worker threads -- far more than there are CPUs."
echo

echo "Step 1-2 (define + reproduce): the over-threaded run."
"$CYCLELAB" compute --duration=2 --threads="$OVER_THREADS" --quiet | print_result

echo
echo "Step 3 (baseline): a correctly-sized run for comparison (--threads=$NCPU)."
"$CYCLELAB" compute --duration=2 --threads="$NCPU" --quiet | print_result

echo
echo "Step 4-6 (classify + hypothesize + pick a measurement): while a longer"
echo "over-threaded run executes, classify where time is going with what's"
echo "already on this machine -- no specialized tool yet:"
if command -v vmstat >/dev/null 2>&1; then
  echo "    vmstat 1 5          # Linux: watch the 'r' (runnable) column"
elif command -v vm_stat >/dev/null 2>&1; then
  echo "    vm_stat 1            # macOS: no run-queue column; pair with ps below"
fi
echo "    ps -eo pid,pcpu,pri,stat,comm | sort -rnk2 | head"
echo "  (run cyclelab with a longer --duration in one terminal, these in another)"

echo
echo "Step 7-10 (change one thing, rerun, check for movement, document):"
echo "  rerun with --threads=$NCPU vs --threads=$OVER_THREADS several times each"
echo "  and compare total throughput, not just wall time, before concluding"
echo "  anything about which configuration is 'faster'."
echo
echo "Interpretation: total throughput across both runs should be roughly"
echo "comparable -- the CPUs can't do more total work just because more"
echo "threads are asking for it -- while the over-threaded run spends more of"
echo "its wall time with threads runnable-but-not-running instead of"
echo "executing. That 'runnable pressure vs. execution' distinction is"
echo "formalized in Chapter 21; here, work through it using only the loop."
