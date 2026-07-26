#!/usr/bin/env bash
#
# Chapter 23 guided lab: check what CPU affinity control is actually
# available on this machine, and -- if none is -- measure natural
# run-to-run variance the scheduler's placement freedom produces instead.
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
REPS="${2:-6}"

echo "Chapter 23 lab, step 1: does this machine support hard CPU affinity?"
echo
"$CYCLELAB" compute --duration=0.2 --threads=2 --affinity=spread --output=/dev/null
RC=$?
echo "(exit code: $RC -- cyclelab continues even when pinning isn't supported)"
echo

OS_NAME="$(uname -s)"
if [ "$OS_NAME" = "Darwin" ]; then
  echo "Step 2: this machine's core topology (relevant even without hard pinning):"
  PCORES=$(sysctl -n hw.perflevel0.physicalcpu 2>/dev/null || echo "?")
  ECORES=$(sysctl -n hw.perflevel1.physicalcpu 2>/dev/null || echo "?")
  THREADS=$(sysctl -n machdep.cpu.thread_count 2>/dev/null || echo "?")
  CORES=$(sysctl -n machdep.cpu.core_count 2>/dev/null || echo "?")
  echo "  performance cores: $PCORES    efficiency cores: $ECORES"
  echo "  thread_count=$THREADS core_count=$CORES -- equal means no SMT on this chip"
  echo
fi

echo "Step 3: without hard pinning, how much does the scheduler's own"
echo "placement freedom show up as run-to-run variance? ($REPS repetitions,"
echo "same command, same everything except whatever the OS decided.)"
echo
printf '%-6s %-18s\n' "rep" "throughput_ops_s"
for ((r = 1; r <= REPS; r++)); do
  OUT="$("$CYCLELAB" compute --duration="$DURATION" --threads=2 --quiet)"
  T=$(printf '%s' "$OUT" | python3 -c \
    "import json,sys; v=json.load(sys.stdin)['results']['throughput_ops_per_s']; print(f'{v:.0f}')")
  printf '%-6s %-18s\n' "$r" "$T"
done

echo
echo "Interpretation: step 1 is this book's honest, tested answer for this"
echo "machine -- cyclelab's --affinity flag reports pinning as unsupported"
echo "and continues rather than failing or silently pretending to pin"
echo "(BLUEPRINT.md Section 8). Step 3's spread is whatever the scheduler's"
echo "own placement decisions contribute to variance when you cannot"
echo "override them -- on a heterogeneous machine (step 2), part of that"
echo "spread can come from threads landing on performance vs. efficiency"
echo "cores differently run to run, something Linux's taskset/sched_setaffinity"
echo "would let you control directly (see the chapter text)."
