#!/usr/bin/env bash
#
# Chapter 8 guided lab: sweep --chains over 1, 2, 4, 8, 16 for the same
# --op=int source-level update workload, several repetitions per chain
# count (median reported), to see the pipeline reward independent work
# up to a point, then behave unevenly rather than flatten cleanly.
#
# Only chain counts that divide 16 evenly are tested: cyclelab's compute
# mode unrolls exactly 16 slots per iteration (Chapter 7), so a chain
# count that doesn't divide 16 evenly (6, 10, 12, 14, ...) hands some
# chains one more update than others within the same iteration -- a
# longer critical path for those chains that has nothing to do with the
# hardware being measured. Testing only divisors of 16 removes that
# confound by construction instead of needing to explain it away.
#
# Repetitions are interleaved round-robin across chain counts (1, 2, 4,
# 8, 16, 1, 2, 4, 8, 16, ...), not run in blocks (all of chain=1, then
# all of chain=2, ...): a blocked order confounds chain count with
# whatever changes over the run's wall-clock duration (thermal state,
# frequency scaling, background load), exactly Chapter 4's own
# interleaved-comparison discipline, applied here to a five-way sweep
# instead of a two-way before/after.
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
REPS="${2:-9}"
CHAIN_LIST=(1 2 4 8 16)

echo "Chapter 8 lab: throughput vs. independent-chain count (--chains), same"
echo "source-level update workload throughout, $REPS repetitions per chain"
echo "count, interleaved round-robin (not run in blocks) to avoid confounding"
echo "chain count with time/thermal drift. Only chain counts dividing 16"
echo "evenly are tested (1, 2, 4, 8, 16) -- see this script's header comment."
echo

# Plain indexed arrays, not associative -- macOS ships bash 3.2 by
# default, which has no associative-array support. RESULTS_N holds every
# CHAIN_LIST[N]'s repetitions, interleaved with every other index's.
NCHAINS=${#CHAIN_LIST[@]}
for ((i = 0; i < NCHAINS; i++)); do
  eval "RESULTS_$i=()"
done

for ((r = 0; r < REPS; r++)); do
  for ((i = 0; i < NCHAINS; i++)); do
    CHAINS="${CHAIN_LIST[$i]}"
    OUT="$("$CYCLELAB" compute --duration="$DURATION" --threads=1 --op=int --chains="$CHAINS" --quiet)"
    T=$(printf '%s' "$OUT" | python3 -c \
      "import json,sys; print(json.load(sys.stdin)['results']['throughput_ops_per_s'])")
    eval "RESULTS_$i+=(\"\$T\")"
  done
done

printf '%-8s %-20s\n' "chains" "median_ops_s"
for ((i = 0; i < NCHAINS; i++)); do
  CHAINS="${CHAIN_LIST[$i]}"
  eval "VALUES=(\"\${RESULTS_$i[@]}\")"
  MEDIAN=$(printf '%s\n' "${VALUES[@]}" | python3 -c \
    "import sys,statistics as st; print(f'{st.median([float(x) for x in sys.stdin]):.0f}')")
  printf '%-8s %-20s\n' "$CHAINS" "$MEDIAN"
done

echo
echo "Interpretation: throughput should rise sharply from 1 chain toward 4,"
echo "then -- rather than settling into a clean plateau -- stay uneven even"
echo "across this fairness-controlled set (every tested value divides 16"
echo "evenly, so no chain count here gets an unfair, longer critical path"
echo "than another). On this book's reference machine, 8 chains measures a"
echo "little below 4, and 16 is the fastest of all. Because every chain"
echo "count's repetitions are interleaved with every other's, this ranking"
echo "cannot be explained by one chain count simply running earlier or later"
echo "in the sweep -- a real confound the blocked-order version of this"
echo "script had until this interleaving was added. That remaining"
echo "unevenness is real and reproducible, but this black-box throughput"
echo "sweep cannot say why -- register allocation, code layout, and other"
echo "compiler-specific choices are all plausible candidates, and pinning"
echo "one down requires reading the generated assembly (cc -S) for the"
echo "specific chain counts in question, not just comparing throughput"
echo "numbers. Do not expect the same favored chain counts on a different"
echo "machine or compiler; do expect the initial 1-to-4 rise and some"
echo "uneven, non-monotonic shape past it, rather than a flat ceiling."
