# labs/scripts

Small helper scripts that back the Guided Lab section of each drafted
chapter. Each script is self-contained, checks its own prerequisites, and
prints an interpretation note rather than a bare number (BLUEPRINT.md
Section 9: "never require the reader to reproduce an exact numeric value").

All scripts that use `cyclelab` expect it to already be built:

```bash
make lab-cyclelab   # from the repo root
```

| Script | Chapter | What it does |
|---|---|---|
| `ch1_time_accounting.sh` | 1 | Compares wall/user/system time for a `cyclelab compute` run. |
| `ch2_size_sweep.sh` | 2 | Runs `cyclelab compute` at three iteration counts and two op mixes, tabulating throughput. |
| `ch3_concurrency_sweep.sh` | 3 | Drives a local `python3 -m http.server` at increasing concurrency (documented stand-in for the not-yet-built `labs/mini-service`). |
| `ch4_interleaved_ab.sh` | 4 | Runs two `cyclelab compute` configurations interleaved, for eyeballing distributions rather than single numbers. |
| `ch5_investigate_slow_config.sh` | 5 | Runs a deliberately over-threaded `cyclelab compute` config alongside `ps`/`vmstat` guidance, for working through the investigation loop. |

Every script is **portable** (BLUEPRINT.md Section 13.2): no root, no
`perf`, no special hardware. They run on Linux and macOS; a few print
platform-appropriate follow-up commands (e.g. `vmstat` vs. `vm_stat`)
rather than pretending one OS's tool exists everywhere.
