#!/usr/bin/env bash
#
# Minimal functional smoke test for labs/cyclelab. Unlike the
# scripts/validate_*.py stubs, this one is real: it builds cyclelab and
# checks that the implemented modes actually run and produce well-formed
# output. BLUEPRINT.md Section 21 ("What CI must not do"): this checks
# correctness and that commands terminate, never a performance threshold.
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CYCLELAB="$REPO_ROOT/labs/cyclelab/bin/cyclelab"

FAIL=0

fail() {
  echo "FAIL: $1" >&2
  FAIL=1
}

echo "== smoke test: labs/cyclelab =="

if [ ! -x "$CYCLELAB" ]; then
  echo "Building cyclelab (release)..."
  make -C "$REPO_ROOT/labs/cyclelab" release >/dev/null 2>&1
fi

if [ ! -x "$CYCLELAB" ]; then
  fail "cyclelab binary not found at $CYCLELAB after build attempt"
  exit 1
fi

echo "-- compute mode --"
OUT="$("$CYCLELAB" compute --duration=1 --threads=2 --quiet)"
RC=$?
if [ "$RC" -ne 0 ]; then
  fail "cyclelab compute exited $RC, expected 0"
else
  echo "  exit code: OK (0)"
fi

if command -v python3 >/dev/null 2>&1; then
  CHECKSUM=$(printf '%s' "$OUT" | python3 -c \
    "import json,sys; d=json.load(sys.stdin); print(d['results']['combined_checksum'])" 2>/dev/null)
  if [ -z "$CHECKSUM" ] || [ "$CHECKSUM" = "None" ]; then
    fail "combined_checksum missing or null in JSON output"
  else
    echo "  JSON parses, combined_checksum present: $CHECKSUM"
  fi
else
  echo "  python3 not found; skipping JSON structure check (exit-code check above still applies)"
fi

echo "-- compute mode, --chains=4 --"
OUT="$("$CYCLELAB" compute --duration=0.5 --threads=1 --chains=4 --quiet)"
RC=$?
if [ "$RC" -ne 0 ]; then
  fail "cyclelab compute --chains=4 exited $RC, expected 0"
else
  echo "  exit code: OK (0)"
fi

echo "-- branch mode --"
OUT="$("$CYCLELAB" branch --duration=0.5 --threads=2 --branch-table-size=50000 --quiet)"
RC=$?
if [ "$RC" -ne 0 ]; then
  fail "cyclelab branch exited $RC, expected 0"
else
  echo "  exit code: OK (0)"
fi
if command -v python3 >/dev/null 2>&1; then
  CHECKSUM=$(printf '%s' "$OUT" | python3 -c \
    "import json,sys; d=json.load(sys.stdin); print(d['results']['combined_checksum'])" 2>/dev/null)
  if [ -z "$CHECKSUM" ] || [ "$CHECKSUM" = "None" ]; then
    fail "branch mode: combined_checksum missing or null in JSON output"
  else
    echo "  JSON parses, combined_checksum present: $CHECKSUM"
  fi
fi

echo "-- random-memory mode --"
OUT="$("$CYCLELAB" random-memory --duration=0.3 --threads=1 --working-set-size=1M --quiet)"
RC=$?
if [ "$RC" -ne 0 ]; then
  fail "cyclelab random-memory exited $RC, expected 0"
else
  echo "  exit code: OK (0)"
fi

echo "-- bandwidth mode --"
OUT="$("$CYCLELAB" bandwidth --duration=0.3 --threads=2 --working-set-size=1M --quiet)"
RC=$?
if [ "$RC" -ne 0 ]; then
  fail "cyclelab bandwidth exited $RC, expected 0"
else
  echo "  exit code: OK (0)"
fi

echo "-- false-sharing mode --"
OUT="$("$CYCLELAB" false-sharing --duration=0.3 --threads=2 --padding=padded --quiet)"
RC=$?
if [ "$RC" -ne 0 ]; then
  fail "cyclelab false-sharing exited $RC, expected 0"
else
  echo "  exit code: OK (0)"
fi

echo "-- stub mode (sleep) --"
"$CYCLELAB" sleep >/dev/null 2>/tmp/cyclelab_smoke_stub_stderr
RC=$?
if [ "$RC" -ne 2 ]; then
  fail "cyclelab sleep exited $RC, expected 2 (recognized-but-unimplemented)"
else
  echo "  exit code: OK (2)"
fi
if ! grep -q "not yet implemented" /tmp/cyclelab_smoke_stub_stderr; then
  fail "cyclelab sleep did not print the expected 'not yet implemented' message"
else
  echo "  stderr message: OK"
fi
rm -f /tmp/cyclelab_smoke_stub_stderr

echo
if [ "$FAIL" -eq 0 ]; then
  echo "smoke test: PASS"
  exit 0
else
  echo "smoke test: FAIL"
  exit 1
fi
