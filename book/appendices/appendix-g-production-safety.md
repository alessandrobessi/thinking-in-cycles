# Appendix G — Production Safety

**Status:** reference material, not a chapter. Collects the operational
discipline implied throughout Chapters 26-29 (dynamic tracing, eBPF,
`bpftrace`/BCC, off-CPU analysis) — the parts of this book capable of
observing a live, real system in the most detail — into one checklist
for using that capability against something that matters.

## Scope probes narrowly

Chapter 26's own mandatory cautions apply here directly: prefer a
stable tracepoint or USDT probe over a raw kprobe/uprobe when one
exists, and always add a predicate (Chapter 28) narrowing exactly which
events matter to the question actually being asked. An unfiltered trace
of a high-frequency event is both an overhead risk (below) and a
signal-to-noise problem — M14 ("more tracing produces more truth"),
directly. The practical rule: state the specific question first, then
build the narrowest probe that answers it, never the reverse.

## Bound duration and output

Any live tracing session against production should have an explicit
time limit and an explicit output-size limit set *before* it starts,
not discovered as a problem after the fact — an unbounded trace against
a hot code path can produce output faster than it can be consumed or
stored, and a session left running past its intended window is a
production risk with no corresponding benefit. Chapter 28's packaged
BCC tools generally take a duration argument directly
(`biolatency 5 1`, five-second interval, one iteration) as their
default interface, for exactly this reason — bounding is the default,
not an opt-in safety measure.

## Prefer aggregation

Chapter 27's Worked Example makes this concrete: storing a latency
histogram in a map and emitting only the finished aggregate, instead of
streaming every individual event to user space, is both the efficient
choice and the safer one — less data leaves the kernel, less data needs
handling and storing, and less of whatever that data might contain
(next section) is ever exposed outside the narrow context it was
captured in. Reach for a ring buffer's per-event streaming (Chapter 27)
only when the individual events themselves, not their aggregate, are
what the question actually needs.

## Estimate overhead

Chapter 26 and 27's shared overhead equation — event rate times work
done per event — should be estimated *before* deploying a probe
broadly, not discovered as an unexplained slowdown afterward. A rough
call-count check against the target hook (how often does this actually
fire, in production, at real traffic levels) is usually enough to
decide whether a given probe is cheap or needs narrowing further before
it's safe to run continuously. M13 ("eBPF has zero overhead") is the
mandatory misconception this discipline exists to correct in practice.

## Protect sensitive data

Chapter 26's fourth mandatory caution stated directly: data captured by
dynamic tracing can include another process's arguments — file paths,
buffer contents, addresses, potentially credentials or personal data
depending on what's being traced. This is exactly why kprobes,
uprobes, and most of eBPF's real capability require elevated privilege
on Linux by default (Appendix A) — the access-control boundary that
protects a process's own memory is the same one that should gate who
can attach a probe to observe it. Treat any output a privileged tracing
tool produces with the same handling care as the logs or data it was
capturing from, not as inherently safe just because it came from a
performance tool rather than the application itself.

## Secure performance tooling

The tooling itself — a `bpftrace` script, a custom eBPF program, a
packaged BCC tool — is running with elevated privilege for as long as
it's attached, which makes its own supply chain part of the security
surface: prefer packaged, maintained tools (Chapter 28's own tool-
choice policy: reach for an existing BCC tool before a hand-written
script) with a known provenance over an unreviewed script copied from
an unfamiliar source, especially for anything that will run with
`CAP_SYS_ADMIN` or root against a production system.

## Retain rollback paths

Chapter 30's final report template lists "rollback or safety
considerations" as its own required field for exactly this reason: any
production-facing change made as a result of an investigation — a
tuning parameter, a code change, a configuration flag — should have a
known, tested way back to the prior state before it ships, independent
of how confident the before/after evidence (Chapter 15) looks. This
applies as much to the *investigation's own tooling* as to the change
it recommends: know how to detach a probe or stop a trace cleanly
before starting it, not only how to start it.

## Communicate operational risk

Chapter 30's final report template also separates "what the data
supports" from "what it does not support" (Chapter 4's own
communicate-honestly discipline, applied to this appendix's territory
specifically): a stakeholder deciding whether to authorize a
privileged tracing session against production needs the estimated
overhead, the bounded duration, the specific data being captured, and
the rollback path stated plainly — not buried in a request to "just run
a quick trace." The same bounded-claim discipline Chapter 30's Key
Takeaway describes for a technical investigation applies directly to
the operational request that makes the investigation possible at all.

## Related

- Chapters 26-28 (the mandatory cautions this appendix operationalizes
  directly: privilege requirements, overhead, predicate scoping,
  sensitive data).
- Chapter 30 (the final report template's safety-relevant fields:
  measurements and overhead, rollback considerations, limitations).
- Appendix A ("safe use of root" — the permissions half of this
  appendix's own data-protection material).
