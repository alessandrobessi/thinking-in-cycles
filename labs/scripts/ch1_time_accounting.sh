#!/usr/bin/env bash
#
# Chapter 1 guided lab: compare wall time, user time, and system time for a
# cyclelab compute run, so elapsed time is not silently equated with CPU
# time. Prefers GNU `time -v` (typically Linux); falls back to the shell's
# built-in `time` (works everywhere, including macOS) when `-v` isn't
# supported.
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CYCLELAB="$SCRIPT_DIR/../cyclelab/bin/cyclelab"

if [ ! -x "$CYCLELAB" ]; then
  echo "cyclelab binary not found at $CYCLELAB -- run 'make lab-cyclelab' from the repo root first." >&2
  exit 1
fi

DURATION="${1:-2}"
THREADS="${2:-2}"

echo "Chapter 1 lab: comparing wall time, user time, and system time"
echo "for 'cyclelab compute --duration=$DURATION --threads=$THREADS'."
echo

if command -v /usr/bin/time >/dev/null 2>&1 && /usr/bin/time -v true >/dev/null 2>&1; then
  /usr/bin/time -v "$CYCLELAB" compute --duration="$DURATION" --threads="$THREADS" \
    --quiet --output=/dev/null
else
  echo "(GNU 'time -v' is not available on this system; falling back to the"
  echo " shell's built-in 'time', which reports real/user/sys but not GNU's"
  echo " fuller breakdown of page faults, context switches, etc.)"
  echo
  time "$CYCLELAB" compute --duration="$DURATION" --threads="$THREADS" \
    --quiet --output=/dev/null
fi

echo
echo "Interpretation: with --threads=$THREADS, user time can add up to roughly"
echo "$THREADS times the wall time if all threads stay busy the whole run --"
echo "wall time and CPU time answer different questions, and neither one"
echo "alone tells you whether the workload is actually CPU-bound in the"
echo "useful-work sense (see Misconceptions M01/M02)."
