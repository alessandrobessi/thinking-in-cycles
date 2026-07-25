#!/usr/bin/env bash
#
# Chapter 7 guided lab (portable fallback): the same instruction mix
# (--op=int, 8 unrolled operations per iteration either way), run once
# with a single dependency chain and once with eight independent chains,
# as an indirect but real demonstration of what differing IPC looks like
# from the outside -- without requiring perf's hardware counters, which
# this lab does not assume are available. See the chapter text for the
# perf stat commands a Linux reader with counter access can run instead.
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CYCLELAB="$SCRIPT_DIR/../cyclelab/bin/cyclelab"

if [ ! -x "$CYCLELAB" ]; then
  echo "cyclelab binary not found at $CYCLELAB -- run 'make lab-cyclelab' from the repo root first." >&2
  exit 1
fi

DURATION="${1:-1}"

echo "Chapter 7 lab: same per-iteration instruction mix, --chains=1 vs --chains=8."
echo

echo "-- chains=1 (one dependency chain: every op waits on the previous one) --"
"$CYCLELAB" compute --duration="$DURATION" --threads=1 --op=int --chains=1 --format=text --quiet

echo
echo "-- chains=8 (eight independent chains: no op waits on a different chain) --"
"$CYCLELAB" compute --duration="$DURATION" --threads=1 --op=int --chains=8 --format=text --quiet

echo
echo "Interpretation: both runs execute the exact same 8 arithmetic operations"
echo "per iteration -- the instruction mix per unit of work is identical."
echo "A large difference in throughput between the two is the same effect a"
echo "hardware counter would report as a large IPC difference: the CPU"
echo "retiring the same kind of instructions much faster per cycle when it"
echo "has independent work to interleave. This lab shows the effect through"
echo "elapsed time and throughput; a reader with 'perf stat' access on Linux"
echo "can see the IPC numbers directly -- see the chapter text for the"
echo "specific commands."
