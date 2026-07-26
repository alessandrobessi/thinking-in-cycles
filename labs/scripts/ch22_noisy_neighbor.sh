#!/usr/bin/env bash
#
# Chapter 22 guided lab: benchmark cyclelab alone, then again while a
# competing "noisy neighbor" cyclelab process shares the machine,
# comparing throughput, variance, and involuntary context switches.
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

REPS="${1:-3}"
DURATION="${2:-1}"
NCPU=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 8)
NEIGHBOR_THREADS=$((NCPU - 2)); [ "$NEIGHBOR_THREADS" -lt 1 ] && NEIGHBOR_THREADS=1

run_and_print() {
  local label="$1"
  "$CYCLELAB" compute --duration="$DURATION" --threads=2 --quiet | python3 -c "
import json, sys
d = json.load(sys.stdin)
r = d['results']
c = r['context_switches']
print(f\"{'$label':<10} throughput={r['throughput_ops_per_s']:<16.0f} nivcsw={c['involuntary']}\")
"
}

echo "Chapter 22 lab: cyclelab compute --threads=2 alone, then with a"
echo "$NEIGHBOR_THREADS-thread noisy neighbor sharing this $NCPU-CPU machine."
echo

echo "-- alone --"
for ((r = 1; r <= REPS; r++)); do
  run_and_print "alone"
done

echo
echo "-- with noisy neighbor --"
NEIGHBOR_DURATION=$((DURATION * (REPS + 2)))
"$CYCLELAB" compute --duration="$NEIGHBOR_DURATION" --threads="$NEIGHBOR_THREADS" \
  --quiet --output=/dev/null &
NEIGHBOR_PID=$!
sleep 0.3
for ((r = 1; r <= REPS; r++)); do
  run_and_print "neighbor"
done
wait "$NEIGHBOR_PID" 2>/dev/null

echo
echo "Interpretation: throughput under the noisy neighbor should be lower"
echo "and involuntary context switches should be substantially higher than"
echo "running alone -- real interference, not just noise, since the neighbor"
echo "is deliberately competing for the same CPUs. This is real,"
echo "reproducible slowdown from co-located, unrelated work, exactly what"
echo "'noisy neighbor' means in a shared environment (a busy build, another"
echo "tenant's VM, a background service) -- not a hypothetical."
