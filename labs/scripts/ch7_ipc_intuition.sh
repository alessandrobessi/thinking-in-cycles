#!/usr/bin/env bash
#
# Chapter 7 guided lab (portable fallback): the same source-level update
# workload (--op=int, 16 fixed update slots per iteration either way --
# an ordinary fixed-trip loop, written to be fully unrollable by the
# optimizer, not manually unrolled in the source), run once with a
# single dependency chain and once with eight independent
# chains, as an indirect but real demonstration of what differing IPC
# looks like from the outside -- without requiring perf's hardware
# counters, which this lab does not assume are available. Note this is
# NOT a verified claim about compiled machine-instruction equivalence --
# chains=1 and chains=8 are separately compiled specializations, and only
# perf stat's own `instructions` counter (with both runs fixed to the
# same --iterations count, not the same --duration) can confirm that. See
# the chapter text for the perf stat commands a Linux reader with counter
# access can run to check.
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CYCLELAB="$SCRIPT_DIR/../cyclelab/bin/cyclelab"

if [ ! -x "$CYCLELAB" ]; then
  echo "cyclelab binary not found at $CYCLELAB -- run 'make lab-cyclelab' from the repo root first." >&2
  exit 1
fi

DURATION="${1:-1}"

echo "Chapter 7 lab: same per-iteration source-level update workload, --chains=1 vs --chains=8."
echo

echo "-- chains=1 (one dependency chain: every op waits on the previous one) --"
"$CYCLELAB" compute --duration="$DURATION" --threads=1 --op=int --chains=1 --format=text --quiet

echo
echo "-- chains=8 (eight independent chains: no op waits on a different chain) --"
"$CYCLELAB" compute --duration="$DURATION" --threads=1 --op=int --chains=8 --format=text --quiet

echo
echo "Interpretation: both runs request the exact same 16 arithmetic updates"
echo "per iteration at the C source level -- the same number and type of"
echo "expressions, verifiable by reading compute.c directly. Whether the"
echo "compiled machine code actually retires the same instruction count is"
echo "a separate, stronger claim this portable lab cannot check -- chains=1"
echo "and chains=8 are separately compiled specializations, and only perf"
echo "stat's own 'instructions' counter (both runs fixed to the same"
echo "--iterations, not the same --duration) can confirm it. A large"
echo "difference in throughput between the two is consistent with a large"
echo "IPC difference; see the chapter text for the perf stat commands that"
echo "actually verify it, in the right order."
