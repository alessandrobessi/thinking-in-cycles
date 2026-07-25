#!/usr/bin/env bash
#
# Chapter 3 guided lab: drive a small HTTP server at increasing concurrency
# and observe the throughput/latency knee where added concurrency mostly
# creates waiting. BLUEPRINT.md's "mini-service" recurring example
# (Section 8) isn't built yet, so this uses python3's built-in HTTP server
# as a documented stand-in -- see labs/mini-service/README.md.
set -uo pipefail

if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 not found; needed to run the stand-in HTTP server." >&2
  exit 1
fi
if ! command -v curl >/dev/null 2>&1; then
  echo "curl not found; needed to generate load." >&2
  exit 1
fi

PORT="${PORT:-8935}"
REQUESTS_PER_BATCH="${REQUESTS_PER_BATCH:-20}"
DOCROOT="$(mktemp -d)"
echo "ok" > "$DOCROOT/index.html"

echo "Chapter 3 lab: throughput as concurrency increases (stand-in HTTP server)."
echo

python3 -m http.server "$PORT" --directory "$DOCROOT" \
  >/tmp/cyclelab_ch3_server.log 2>&1 &
SERVER_PID=$!
cleanup() {
  kill "$SERVER_PID" >/dev/null 2>&1
  wait "$SERVER_PID" 2>/dev/null   # reap quietly; suppresses the shell's "Terminated" job message
  rm -rf "$DOCROOT"
}
trap cleanup EXIT

READY=0
for _ in $(seq 1 30); do
  if curl -s -o /dev/null "http://127.0.0.1:$PORT/"; then
    READY=1
    break
  fi
  sleep 0.1
done
if [ "$READY" -ne 1 ]; then
  echo "server did not become ready on port $PORT; see /tmp/cyclelab_ch3_server.log" >&2
  exit 1
fi

printf '%-12s %-14s %-18s\n' "concurrency" "elapsed_s" "throughput_req_s"
for CONC in 1 4 16 64; do
  START=$(python3 -c 'import time; print(time.time())')
  for ((batch = 0; batch < REQUESTS_PER_BATCH; batch++)); do
    batch_pids=()
    for ((i = 0; i < CONC; i++)); do
      curl -s -o /dev/null "http://127.0.0.1:$PORT/" &
      batch_pids+=("$!")
    done
    # Wait only for this batch's curls -- a bare `wait` would also block on
    # the long-running http.server background job started above.
    wait "${batch_pids[@]}"
  done
  END=$(python3 -c 'import time; print(time.time())')
  TOTAL=$((CONC * REQUESTS_PER_BATCH))
  python3 -c "
elapsed = $END - $START
total = $TOTAL
print(f'{\"$CONC\":<12} {elapsed:<14.3f} {total/elapsed:<18.1f}')
"
done

echo
echo "Interpretation: throughput should rise with concurrency at first, then"
echo "flatten (or fall) once this server and OS run out of slack -- the 'knee'"
echo "the chapter describes. This toy server does almost no work per request,"
echo "so treat the exact concurrency where it flattens as specific to this"
echo "machine and this run, not a general threshold (Misconception M22)."
