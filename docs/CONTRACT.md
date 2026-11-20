# Team Contract: CampusCompute

Two people, two sides. Read once, sign at the bottom (type your names), commit.

## 1. Ownership (only the owner edits these files)

| Teammate (T) | You (Y) |
|---|---|
| `src/job.c`, `include/job.h` | `src/main.c` |
| `src/scheduler.c`, `include/scheduler.h` | `src/logger.c`, `include/logger.h` |
| `src/banker.c`, `include/banker.h` | `src/ipc.c`, `include/ipc.h` |
| `src/worker.c`, `include/worker.h` | `src/res_limits.c`, `include/res_limits.h` |
| `tests/jobs.txt`, `invalid_jobs.txt`, `banker_*.txt` | `src/demo.c`, `include/demo.h` |
| | `Makefile`, `README.md`, `.gitignore`, `docs/` |

- Need a change in the other person's file? Ask them (message or GitHub issue). Never edit it yourself.
- T may add new `.c` files in `src/` and `.h` in `include/`. The Makefile picks them up automatically.
- Modules must not depend on each other's code. Only `demo.c` and `main.c` call across modules.

## 2. Interface (frozen, do not change signatures)

Every module exposes exactly one entry function, already declared in its header:

| Function | Owner | Called as |
|---|---|---|
| `int run_sched(int argc, char **argv)` | T | argv = `{"sched", "fcfs"/"rr"/"prio"/"compare", "<jobs file>"}` |
| `int run_banker(int argc, char **argv)` | T | argv = `{"banker", "<state file>"}` |
| `int run_worker(int argc, char **argv)` | T | argv = `{"worker", "ok"/"fail"}` |
| `int run_logger(int argc, char **argv)` | Y | argv = `{"logger"}` |
| `int run_ipc(int argc, char **argv)` | Y | argv = `{"ipc"}` |
| `int run_limits(int argc, char **argv)` | Y | argv = `{"limits"}` |
| `int run_demo(int argc, char **argv)` | Y | argv = `{"demo"}` |

`argv[0]` is the subcommand name, same as `main.c` passes it.

## 3. Behaviour rules for T's functions (so `demo` can call them)

1. **Return 0** when the feature worked correctly, including when it correctly *detects* a problem
   (Banker UNSAFE state, worker child failing). Return **1** for a real runtime error, **2** for bad usage or bad input.
2. **Never call `exit()` in a `run_*` function** (only in a forked child). `demo` must continue after your function returns.
3. **Callable repeatedly** in one process: no leftover global state, free memory, close files.
4. Normal output to **stdout**. Errors to **stderr**, formatted `Error: <reason>`.
5. Reap every child with `waitpid()`. No zombies.

## 4. Fixed behaviour (from our design decisions)

- Scheduling: tie-break by lower job ID. Round Robin quantum = **4**. Print a Gantt line, a per-job table
  (CT, TAT, WT) and the averages. `compare` prints all three algorithms side by side.
- Worker `fail`: child prints `SIMULATED FAILURE` and calls `exit(3)`. The parent prints that it detected exit code 3.
- Banker output must contain the word **SAFE** with the safe sequence, or **UNSAFE**.

## 5. File formats (frozen)

Lines starting with `#` and blank lines are ignored.

- **Jobs file:** one job per line: `id arrival burst priority` (all integers; burst > 0, others >= 0, unique ids).
- **Banker file:** first data line `P R`. Then Available (R numbers), Allocation (P rows of R), Maximum (P rows of R).
  See `tests/banker_safe.txt`.

## 6. Git rules

- Private repo, both as collaborators. `main` always compiles.
- Branches: `teammate` and `you` (or `feature-*`). Merge into `main` by pull request, reviewed by the other person.
- `git pull` before starting work each day. No force-push to `main`. Never commit `build/`, binaries or logs.
- Commit message format: `module: what changed` (e.g. `scheduler: add round robin`).
- `make` must give zero errors and zero warnings before you open a pull request.

## 7. Schedule and sync points

| Date | Milestone |
|---|---|
| Day 3 | First working version of each module pushed to own branch |
| Day 5 | Quick call: compare outputs against the contract |
| Day 6 | Merge everything into `main`, swap and test each other's modules |
| Day 8 | **Code freeze.** Only bug fixes agreed by both |
| Day 9 | Each person takes their own screenshots with their own SRN |

If you will miss a milestone, say so at least a day before.

## 8. Academic integrity

- Each person must be able to explain **every** file in the project, including the other person's.
- Each person submits their own PDF with their own SRN and own screenshots.
- No code copied from other teams or from the internet without understanding it.

## Sign-off

Teammate: ____________________  Date: ________
You: ____________________  Date: ________

## 10. Amendment: teammate's actual implementation (replaces parts of sections 2-4)

The teammate implemented function-style modules instead of `run_*` entry points. Instead of rewriting them, `src/adapters.c` (owned by You) bridges the two:

| CLI command | Adapter | Teammate function(s) called |
|---|---|---|
| `sched fcfs/rr/prio/compare <file>` | `run_sched` | `initializeJobs`, `validateJob`, `displayJobs`, `fcfsScheduling`, `roundRobinScheduling(jobs, n, 4)`, `priorityScheduling`, `updateJobStatus` |
| `banker <file>` | `run_banker` | `bankerSafety` |
| `worker ok/fail` | `run_worker` | `runWorker(0/1)` |

Changes to the earlier rules:
- File parsing and validation for the jobs file and Banker's file live in `adapters.c`, so teammate functions keep their array-based signatures.
- Section 2's `run_sched`, `run_banker`, `run_worker` signatures are now declared in `include/adapters.h`, not in the teammate's headers.
- The simulated worker failure exits with code **1** (not 3). It is labelled as simulated in the adapter output and in `worker.c`.
- Tie-breaking rule: equal arrival times are ordered by lower job ID. Priority picks the smaller priority number, then earlier arrival, then lower job ID. Round Robin quantum = 4.

Fixes applied to the teammate's files (listed in `teammate_changes.diff`):
1. `job.c`: removed a stray `main()` that clashed with `src/main.c` (linker error).
2. `job.c`: `removeJob` compared an int with the pointer `n` (`i < n-1`); now `i < *n - 1`.
3. `scheduler.c`: `sortJobsByArrival` now breaks arrival ties by job ID, matching the stated rule.

The teammate owns these files and must apply the same three changes in their own copy.
