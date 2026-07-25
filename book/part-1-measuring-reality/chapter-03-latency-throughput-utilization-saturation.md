# Chapter 3 — Latency, Throughput, Utilization, and Saturation

**Part:** Part I — Measuring Reality
**Concept level:** 0
**Prerequisites:** workload, operation, workload model, metric (Chapters 1-2)
**New concepts:** latency, throughput, utilization, saturation, concurrency, queue, tail latency

## Opening Question

How do latency, throughput, utilization, and saturation differ?

## Incident or Real-World Story

A team doubles the worker-thread count on a busy internal API because a
dashboard shows the current pool "only" handling 800 requests per second
and they want more. After the change, throughput does go up — briefly, to
about 950 requests per second — but p99 latency, which had been a steady
40ms, climbs past 400ms and stays there. Support tickets about a "slow"
service start arriving within the hour, even though the team just added
capacity.

What happened is that the service was already close to saturated before
the change: its downstream database connection pool could sustain roughly
the same amount of concurrent work regardless of how many API worker
threads were asking it for more. Adding more worker threads didn't add
more usable capacity to the actual constrained resource — it added more
requests competing for the same limited pool, so more of them sat waiting
in a queue instead of being served promptly. Throughput crept up slightly
because a few more requests squeezed through, but the average and tail
latency both got much worse, because now there was a longer line for
everyone. The team's dashboard had been showing them utilization and
raw throughput. It never showed them the queue.

## Predict Before Measuring

Before reading further: if you drive a fixed-capacity system with more
and more concurrent requests, what do you expect to happen to throughput?
Does it rise forever, rise then flatten, or rise then fall? What do you
expect to happen to latency over the same range? Write down both
predictions before the Guided Lab.

## Worked Example

A supermarket checkout line is the cleanest version of this chapter's
whole vocabulary. Each cashier can process a certain number of customers
per hour — that rate, summed across all open registers, is the store's
**throughput**. How long any one customer spends from the back of the
line to walking out with their bags is their **latency**. **Utilization**
is the fraction of time each cashier is actively busy versus idle. And
**saturation** is what happens once the line starts growing faster than
cashiers can drain it: even a cashier who is busy 100% of the time (fully
utilized) can preside over a line that keeps getting longer, if customers
are arriving faster than that cashier — or all the cashiers together —
can check them out.

A store can serve many customers per hour in aggregate while one
particular customer still waits far too long, and a cashier can be busy
without the *store* being at its maximum sustainable throughput. All four
quantities are real, all four are useful, and none of them can substitute
for the other three.

## Core Intuition

**Latency** is how long one operation takes, start to finish. **Throughput**
is how much work completes per unit of time. **Utilization** is the
fraction of available time a resource spent busy. **Saturation** is the
point past which additional concurrent work mostly increases waiting
rather than completed work — it shows up as a growing **queue**, work
that has arrived but hasn't started yet. **Concurrency** is simply how
much work is in flight — running or waiting — at once. **Tail latency**
is the latency of the *worst* handled requests (commonly reported as a
p95, p99, or p999 percentile), which matters because a small fraction of
very slow requests can still represent a large fraction of a system's
worst user experiences, even while the average looks perfectly healthy.

The informal intuition worth carrying forward, sometimes called Little's
Law: the average number of requests in a system tends to equal the
average arrival rate times the average time each one spends in the
system. You don't need the formal queueing-theory version of that
statement to use it — the qualitative version is enough: if latency goes
up while the arrival rate stays the same, there must be more work in
flight inside the system than before, and that work is queued somewhere.

## Technical Explanation

Utilization and saturation are frequently conflated, and the story above
is exactly that conflation in practice. A resource can be moderately
utilized — busy, say, 60% of the time — and still be saturated, if
arrivals are bursty or correlated rather than perfectly smooth: a burst of
concurrent requests can queue up even though the resource's *average*
utilization across the whole minute looks unremarkable. Conversely, a
resource can run at 100% utilization on purpose and that can be entirely
correct — a batch job intentionally trying to keep every CPU maximally
busy wants high utilization; that is its goal, not a warning sign.

Throughput as concurrency increases typically follows a recognizable
shape: it rises as more concurrent work lets a system overlap waiting
with useful execution, then flattens once the system's actual constrained
resource (a connection pool, a lock, a CPU core, a disk) is saturated,
and can even fall past that point as the overhead of managing more
queued, waiting work starts competing with the work of actually doing
it. The "knee" of that curve — where the flattening begins — is the point
past which more concurrency stops helping throughput and starts only
adding latency, exactly as it did for the API in this chapter's story.

## Tool View

- What is measured: throughput and latency at increasing levels of
  concurrent load against the same fixed system.
- What is not measured: *why* the knee happens at the concurrency level
  it does — that requires identifying the specific saturated resource,
  which needs tools from later chapters (Part III onward) once the
  problem is inside a process rather than in the shape of a curve.
- Required permissions: none beyond running the workload and a client
  against it.
- Likely overhead: the load-generation client itself consumes CPU and
  can become the bottleneck at very high concurrency — a confounder this
  chapter's lab is too small in scale to hit, but worth remembering.
- Portability: this chapter's lab uses `python3 -m http.server` and
  `curl`, both commonly available; no `mini-service` (BLUEPRINT.md's
  planned second recurring example) exists yet — see
  `labs/mini-service/README.md`.
- Common failure mode: reading "throughput went up" alone as "things got
  better," without checking what happened to latency at the same time.

## Guided Lab

**Portability:** portable.

**Setup:** requires `python3` and `curl` (both commonly preinstalled).

**Command:**

```bash
./labs/scripts/ch3_concurrency_sweep.sh
```

This starts a local, single-purpose HTTP server, then drives it with
`curl` at increasing concurrency levels (1, 4, 16, 64 simultaneous
requests), tabulating elapsed time and throughput at each level.

**Expected qualitative result:** throughput should rise from concurrency
1 to some middle level, then flatten or fall by the highest level tested.
One example run on the reference machine for this book showed:

```text
concurrency  elapsed_s   throughput_req_s
1            0.115       173.6
4            0.174       460.0
16           0.400       799.9
64           2.710       472.2
```

Throughput roughly quadrupled going from concurrency 1 to 4, still rose
(more slowly) from 4 to 16, then *fell* by concurrency 64 — the knee, and
past it, on this machine and this run. Expect the specific concurrency
level where this happens to vary by machine; do not expect these exact
numbers.

**Interpretation:** this toy server does almost no work per request, so
the "resource" that eventually saturates is mostly the local machine's
own ability to spawn and manage many concurrent connections and
processes — not a realistic production bottleneck. That's fine for
seeing the *shape* of the curve, which is the chapter's point; identifying
a *specific* real bottleneck behind a knee like this is what the rest of
the book builds toward.

**Cleanup:** the script stops its own server and removes its temporary
directory automatically on exit.

**Fallback path:** if `python3` or `curl` isn't available, the same
qualitative point can be observed with any HTTP client and any way of
issuing concurrent requests (e.g. `ab`, `hey`, or a handful of background
`wget` loops) against any locally running web server — the specific
tools matter far less than driving one fixed server at increasing
concurrency and comparing throughput and elapsed time at each level.

## Common Misconceptions

**M22 (proposed) — "100% utilization is always bad, and any utilization
below 100% means there's no problem."** This is wrong because utilization
and saturation are different measurements: a resource can be moderately
utilized and still heavily saturated under bursty or correlated arrivals,
and full utilization is sometimes the deliberate goal, as with a batch
job trying to maximize throughput. The evidence that distinguishes the
two: drive a system at increasing concurrency and plot utilization
alongside queue growth or latency — saturation can appear before
utilization visibly reaches 100%, and utilization near 100% by itself is
not evidence of a problem for a throughput-oriented workload.

## Practical Implications

When a system is "slow," ask separately: what is its current throughput,
what is its current latency (including the tail, not just the average),
what is its bottleneck resource's utilization, and is that resource
saturated. These four answers are not redundant with each other, and
"we added capacity" only helps if the capacity added is the resource that
was actually saturated — otherwise, as in this chapter's story, it can
make the tail worse while barely moving the average.

## Key Takeaway

**More work in flight can raise throughput until the system saturates,
after which it mostly raises latency.**

## What to Remember

- Latency is about one operation; throughput is about a rate of
  completed work; neither substitutes for the other.
- Utilization is how busy a resource is; saturation is whether work is
  queuing up faster than it's being drained — they are not the same
  measurement.
- A resource can be saturated without being 100% utilized, under bursty
  arrivals.
- 100% utilization is sometimes the correct goal, not a warning sign.
- Throughput under increasing concurrency typically rises, flattens at a
  knee, and can fall past that point.
- Tail latency (p95/p99/p999) can be bad even while average latency looks
  fine.
- Adding capacity only helps if it targets the actual saturated resource.

## Further Reading

- Brendan Gregg, *Systems Performance: Enterprise and the Cloud*, 2nd ed.
  — utilization/saturation/errors framing (the USE Method) underlies this
  chapter's core distinction. See `references/bibliography.md`.
- Neil J. Gunther's writing on queueing intuition and Little's Law is the
  standard non-formal reference for the "arrivals × time-in-system"
  relationship used informally in this chapter. See
  `references/bibliography.md`.

## The Next Obvious Question

How do we know a measured difference is real?
