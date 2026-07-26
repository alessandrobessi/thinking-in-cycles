# Part II — What the CPU Is Doing

Connects source code to the machine instructions a CPU actually executes
(Chapter 6), the cycle/instruction ratios used to reason about that
execution (Chapter 7), why a CPU can stall despite having work queued up
(Chapter 8), how branch prediction and speculation interact with that
same pipeline (Chapter 9), and how to turn all of it into a
hypothesis-driven measurement with `perf stat` (Chapter 10).

| Chapter | Title | Opening Question |
|---|---|---|
| 6 | [From Source Code to Retired Instructions](chapter-06-from-source-code-to-retired-instructions.md) | What work does the CPU actually execute? |
| 7 | [Cycles, Instructions, IPC, and CPI](chapter-07-cycles-instructions-ipc-and-cpi.md) | What do cycles and instructions tell us? |
| 8 | [The Pipeline: Front End, Back End, and Stalls](chapter-08-the-pipeline-front-end-back-end-and-stalls.md) | Why can a CPU spend cycles without retiring useful work? |
| 9 | [Branch Prediction, Speculation, and Dependencies](chapter-09-branch-prediction-speculation-and-dependencies.md) | How do branches and dependencies disrupt execution? |
| 10 | [`perf stat`: Turning Counters into a Hypothesis](chapter-10-perf-stat-turning-counters-into-a-hypothesis.md) | How can counters turn a vague slowdown into a hypothesis? |

Guided-lab portability is mixed in this Part, unlike Part I: Chapters 6-9
have **portable** primary labs built on `labs/cyclelab`'s `compute`
(including its `--chains` flag) and `branch` modes — no root, no `perf`
required. Chapter 10's lab is **hardware-dependent / privileged**
(Linux with counter access); its portable fallback reuses Chapter 7's
lab as indirect evidence of the same effect `perf stat` would show
directly. None of this Part's `perf stat` commands were tested against
real output on this book's reference machine (macOS, where `perf`
doesn't exist) — they're documented against `perf`'s stable, published
interface and clearly marked schematic where shown.

Next: [Part III — Where the CPU Time Goes](../part-3-where-cpu-time-goes/README.md) (Chapters 11-15).
