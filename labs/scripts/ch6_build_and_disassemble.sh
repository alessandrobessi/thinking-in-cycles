#!/usr/bin/env bash
#
# Chapter 6 guided lab: build cyclelab at -O0 and -O2 (already how
# `make debug`/`make release` work) and compare the compiled instruction
# count of the same function, compute_worker, between the two builds.
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/.."
CYCLELAB_DIR="$REPO_ROOT/labs/cyclelab"

if ! command -v objdump >/dev/null 2>&1; then
  echo "objdump not found. On Linux, install binutils (e.g. apt install" >&2
  echo "binutils / dnf install binutils). On macOS, install Xcode Command" >&2
  echo "Line Tools (xcode-select --install)." >&2
  exit 1
fi

echo "Chapter 6 lab: same function, two optimization levels."
echo

echo "Building debug (-O0 -g) and release (-O2 -g) variants..."
make -C "$CYCLELAB_DIR" debug release >/dev/null

OS_NAME="$(uname -s)"
if [ "$OS_NAME" = "Darwin" ]; then
  OBJDUMP_FLAGS="-d --macho"
  SYMBOL_LABEL="_compute_worker:"
else
  OBJDUMP_FLAGS="-d"
  SYMBOL_LABEL="<compute_worker>:"
fi

extract_function() {
  local binary="$1"
  if [ "$OS_NAME" = "Darwin" ]; then
    objdump $OBJDUMP_FLAGS "$binary" 2>/dev/null | \
      awk -v label="$SYMBOL_LABEL" '$0==label{flag=1;next} /^_[a-zA-Z_]+:$/{flag=0} flag'
  else
    objdump $OBJDUMP_FLAGS "$binary" 2>/dev/null | \
      awk -v label="$SYMBOL_LABEL" 'index($0,label){flag=1;next} /^$/{flag=0} flag'
  fi
}

DEBUG_BIN="$CYCLELAB_DIR/bin/cyclelab-debug"
RELEASE_BIN="$CYCLELAB_DIR/bin/cyclelab"

extract_function "$DEBUG_BIN" > /tmp/cyclelab_ch6_debug.txt
extract_function "$RELEASE_BIN" > /tmp/cyclelab_ch6_release.txt

DEBUG_COUNT=$(wc -l < /tmp/cyclelab_ch6_debug.txt | tr -d ' ')
RELEASE_COUNT=$(wc -l < /tmp/cyclelab_ch6_release.txt | tr -d ' ')

echo
echo "compute_worker() static instruction count:"
printf '  -O0 (debug):   %s instructions\n' "$DEBUG_COUNT"
printf '  -O2 (release): %s instructions\n' "$RELEASE_COUNT"
echo
echo "First 10 disassembled instructions of each, for a structural look"
echo "(register names and exact instructions are architecture-specific --"
echo " expect different mnemonics entirely on x86-64 vs Arm64):"
echo
echo "-- -O0 --"
head -10 /tmp/cyclelab_ch6_debug.txt
echo
echo "-- -O2 --"
head -10 /tmp/cyclelab_ch6_release.txt

echo
echo "Interpretation: expect fewer static instructions at -O2 than -O0 for"
echo "the same function, and different register usage (the optimized build"
echo "keeps more values in registers across the loop instead of reloading"
echo "them from the stack every time, which -O0 does almost everywhere)."
echo "Do not expect the exact counts above -- they are architecture- and"
echo "compiler-version-specific; the direction (fewer, more register-heavy"
echo "instructions at -O2) is the qualitative result to look for."

rm -f /tmp/cyclelab_ch6_debug.txt /tmp/cyclelab_ch6_release.txt
