#!/usr/bin/env bash
#
# Chapter 13 guided lab: build the same workload three ways -- full debug
# info, no debug info, and frame-pointer-omitted -- and compare what a
# real profiler (macOS `sample`) can and cannot reconstruct from each.
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CYCLELAB_SRC_DIR="$SCRIPT_DIR/../cyclelab"
BIN_DIR="$CYCLELAB_SRC_DIR/bin"

if ! command -v sample >/dev/null 2>&1; then
  echo "'sample' not found -- this lab is macOS-specific. See the chapter text for the" >&2
  echo "equivalent frame-pointer/debug-symbol comparison on Linux with perf." >&2
  exit 1
fi

DURATION="${1:-3}"
SAMPLE_SECONDS="${2:-2}"
SRC="$CYCLELAB_SRC_DIR/src"
SOURCES=("$SRC"/*.c "$SRC"/modes/*.c)

mkdir -p "$BIN_DIR"

echo "Chapter 13 lab: building three variants of the same source..."
cc -std=c11 -O2 -g -o "$BIN_DIR/ch13-full" "${SOURCES[@]}" -lpthread
cc -std=c11 -O2    -o "$BIN_DIR/ch13-nodebug" "${SOURCES[@]}" -lpthread
cc -std=c11 -O2 -g -fomit-frame-pointer -o "$BIN_DIR/ch13-nofp" "${SOURCES[@]}" -lpthread
echo

profile_one() {
  local label="$1" binary="$2"
  echo "== $label =="
  "$binary" compute --duration="$DURATION" --threads=1 --chains=1 --quiet --output=/dev/null &
  local cpid=$!
  sleep 0.3
  local raw
  raw="$(mktemp)"
  sample "$cpid" "$SAMPLE_SECONDS" -f "$raw" >/dev/null 2>&1
  wait "$cpid" 2>/dev/null

  echo "  main thread ancestry (does it reach back to 'main'/'start'?):"
  awk '
    /^Call graph:/ { ingraph=1; next }
    ingraph && /^    [0-9]+ Thread/ { threadcount++; if (threadcount > 1) exit }
    ingraph { print "    " $0 }
  ' "$raw"

  echo "  source-line info present on compute_worker samples?"
  if grep -q 'compute_worker.*compute\.c:[1-9]' "$raw"; then
    echo "    yes -- specific line numbers appear (e.g. compute.c:88)"
  else
    echo "    no -- compute_worker samples carry no non-zero source line"
  fi
  rm -f "$raw"
  echo
}

profile_one "full debug info (-g), normal frame pointers" "$BIN_DIR/ch13-full"
profile_one "no debug info (no -g)" "$BIN_DIR/ch13-nodebug"
profile_one "debug info present, but -fomit-frame-pointer" "$BIN_DIR/ch13-nofp"

echo "Interpretation: the full-debug-info build should show both readable"
echo "source lines AND an unbroken main-thread ancestry back through 'main'"
echo "and 'start'. The no-debug-info build should still show function"
echo "*names* (they come from the symbol table, independent of debug info)"
echo "but no source-line numbers. The frame-pointer-omitted build should"
echo "show source lines fine, but a main-thread ancestry that stops short"
echo "-- missing 'main' and 'start' -- because the unwinder had nothing"
echo "reliable to walk past compute_run without frame pointers to follow."
echo
echo "Do not expect identical behavior on every OS/toolchain: how much a"
echo "missing frame pointer actually breaks unwinding depends on the"
echo "platform's unwind-table support (BLUEPRINT.md's own policy: do not"
echo "mandate frame pointers universally -- the tradeoff is real and"
echo "platform-dependent, not a one-size-fits-all rule)."
