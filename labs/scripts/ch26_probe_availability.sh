#!/usr/bin/env bash
#
# Chapter 26 guided lab: what does this machine actually offer for
# event-driven, per-call-site tracing (tracepoints/kprobes/kretprobes/
# uprobes/USDT)? Reports real, tested availability rather than assuming
# Linux's tools exist everywhere.
set -uo pipefail

echo "Chapter 26 lab: what can this machine actually attach a probe to?"
echo

OS_NAME="$(uname -s)"

echo "Step 1: dtrace -- macOS's own dynamic tracing framework, the closest"
echo "conceptual relative to Linux kprobes/uprobes/USDT on this machine."
echo
if [ "$OS_NAME" = "Darwin" ]; then
  if command -v dtrace >/dev/null 2>&1; then
    OUT=$(dtrace -l 2>&1 >/dev/null)
    echo "  \$ dtrace -l"
    echo "$OUT" | sed 's/^/  /'
    echo
    if echo "$OUT" | grep -qi "additional privileges\|system integrity protection"; then
      echo "  Result: dtrace is present but this SIP-enabled system refuses to"
      echo "  list probes without elevated privileges -- a real, reproducible"
      echo "  example of Chapter 26's own caution: dynamic tracing frequently"
      echo "  requires privilege because it can observe any process's"
      echo "  arguments, including sensitive ones."
    else
      echo "  Result: dtrace listed probes without elevated privileges on this"
      echo "  machine -- more than this book's own reference machine could do."
    fi
  else
    echo "  dtrace not found on this system."
  fi
else
  echo "  Skipping: this is a Linux-family check normally, but this script"
  echo "  is reporting from a $OS_NAME machine's own guided-lab run."
fi
echo

echo "Step 2: bpftrace / BCC -- Chapter 28's tools, Linux-only."
echo
if command -v bpftrace >/dev/null 2>&1; then
  echo "  bpftrace found: $(bpftrace --version 2>/dev/null || echo present)"
else
  echo "  bpftrace not found on this system."
fi
echo

echo "Step 3: what this means for the rest of this Part."
echo
echo "  Run 'make doctor' for the full, authoritative picture (kernel BTF,"
echo "  perf_event_paranoid, kptr_restrict, and this same dtrace/bpftrace"
echo "  check, all in one place)."
echo
echo "  Neither dtrace nor bpftrace is usable, unprivileged, on this book's"
echo "  own macOS reference machine. Chapters 26-28's Linux commands are"
echo "  therefore documented against each tool's stable, versioned"
echo "  interface, not tested against real captured output here -- clearly"
echo "  marked wherever they appear. What IS real and tested throughout"
echo "  this Part: cyclelab's own workloads (lock-contention, sleep) and"
echo "  macOS's sample(1), which captures every thread's stack on a"
echo "  wall-clock interval regardless of run state -- close enough to an"
echo "  event-driven view to make Chapter 29's off-CPU lab genuinely real."
