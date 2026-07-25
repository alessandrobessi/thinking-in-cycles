# labs/mini-service

BLUEPRINT.md Section 8 calls for a second recurring example: "a small
request-serving application, such as an HTTP key-value service, used only
when queueing, tail latency, concurrency, or off-CPU behavior needs a more
realistic workload than `cyclelab` provides."

**Not implemented in this phase.** Chapter 3's guided lab, which is the
first place the blueprint calls for this kind of workload, currently uses
`python3 -m http.server` plus a `curl` load loop as a documented stand-in
(see `labs/scripts/ch3_concurrency_sweep.sh`). That substitute is enough to
demonstrate the throughput/latency knee at increasing concurrency, but it
has no real per-request work, no tail-latency behavior to speak of, and no
queueing model beyond the OS's own TCP backlog -- it will not be adequate
once the book reaches concurrency- and tail-latency-focused material in
later parts.

Build `mini-service` before drafting any chapter whose lab depends on
realistic queueing, tail latency, or off-CPU behavior under load.
