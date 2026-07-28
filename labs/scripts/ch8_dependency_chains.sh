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
# Repetitions are collected in a freshly randomized order every round
# (not a fixed round-robin, and not run in blocks): a *fixed* rotating
# order (1, 2, 4, 8, 16, 1, 2, 4, 8, 16, ...) still confounds chain
# count with within-round position -- whichever chain count always runs
# first could differ from the others because of a real, repeatable
# per-position effect (frequency ramp-up, cache state at the start of a
# round), not because of chain count itself. Randomizing which chain
# count lands in which position, fresh every round, is what actually
# rules that out -- the same logic as randomized block design generally,
# applied here to a five-way sweep instead of a two-way before/after.
set -uo pipefail
# (Not set -e: this project's lab scripts consistently avoid it --
# set -e's behavior inside command substitutions and conditional
# contexts is a well-known footgun, and this script instead validates
# each parsed value explicitly, below, rather than relying on it.)

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
echo "count, each round in a freshly randomized order to avoid confounding"
echo "chain count with within-round position or sweep-wide drift. Only chain"
echo "counts dividing 16 evenly are tested (1, 2, 4, 8, 16) -- see this"
echo "script's header comment."
echo

# Plain indexed arrays, not associative -- macOS ships bash 3.2 by
# default, which has no associative-array support. RESULTS_N holds every
# CHAIN_LIST[N]'s repetitions, collected in randomized order.
NCHAINS=${#CHAIN_LIST[@]}
for ((i = 0; i < NCHAINS; i++)); do
  eval "RESULTS_$i=()"
done

for ((r = 0; r < REPS; r++)); do
  ORDER=$(python3 -c "import random; idx=list(range($NCHAINS)); random.shuffle(idx); print(' '.join(map(str, idx)))")
  for i in $ORDER; do
    CHAINS="${CHAIN_LIST[$i]}"
    OUT="$("$CYCLELAB" compute --duration="$DURATION" --threads=1 --op=int --chains="$CHAINS" --quiet)"
    T=$(printf '%s' "$OUT" | python3 -c \
      "import json,sys; print(json.load(sys.stdin)['results']['throughput_ops_per_s'])")
    case "$T" in
      ''|*[!0-9.]*)
        echo "cyclelab: unexpected non-numeric throughput '$T' for chains=$CHAINS (rep $r) -- aborting" >&2
        exit 1
        ;;
    esac
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
echo "Interpretation: throughput should rise as chain count increases from 1,"
echo "then the gains should taper off -- the portable part of this lab's"
echo "result. The exact shape past that point (a clean plateau, a gradual"
echo "decline, or something uneven) is not itself portable -- it depends on"
echo "the specific compiler and microarchitecture, and this black-box"
echo "throughput sweep can observe that shape but not explain it. On this"
echo "book's reference machine, the measured shape is uneven rather than a"
echo "clean plateau: 8 chains measures a little below 4, and 16 is the"
echo "fastest of all. Because every round's chain-count order is freshly"
echo "randomized, this ranking is very unlikely to be explained by one"
echo "chain count simply occupying a favorable position within each round,"
echo "or by a slow drift across the whole sweep -- both were real confounds"
echo "earlier versions of this script had. Randomization reduces that risk;"
echo "it doesn't prove it away -- with only a handful of rounds, a residual"
echo "by-chance imbalance remains possible in principle. Pinning down *why*"
echo "this specific machine favors these specific chain counts requires"
echo "reading the generated assembly (cc -S) for them, not just comparing"
echo "throughput numbers."
