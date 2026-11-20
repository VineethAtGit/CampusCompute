# CampusCompute
OS Level 3 Mini-Project (UE25CS242A): job scheduling and resource management in C.

## Prerequisites
Ubuntu / WSL Ubuntu with `gcc` and `make`.

## Build
    make
## Run
    ./campuscompute help
## Clean
    make clean

## Test
    make test      # unit tests + command-line tests
    make unit      # unit tests only (job, scheduler sort, Banker's)
    make cli       # command-line tests only (every command, edge cases, invalid input, cleanup)

`make test` builds what it needs. All tests must pass before a pull request.

## Layout
- `src/`, `include/` : source and headers (one module per requirement)
- `tests/` : repeatable input files, `unit_tests.c` (C unit tests) and `run_tests.sh` (command-line tests)
- `logs/` : generated logs
- `docs/` : checklist and screenshot working folder

## Team
- Teammate: job.c, scheduler.c, banker.c, worker.c (algorithms)
- Owner of CLI, logger, IPC, limits, demo, Makefile: see docs/CONTRACT.md

## Commands
    ./campuscompute sched fcfs|rr|prio|compare tests/jobs.txt
    ./campuscompute logger
    ./campuscompute ipc
    ./campuscompute banker tests/banker_safe.txt
    ./campuscompute banker tests/banker_unsafe.txt
    ./campuscompute worker ok|fail
    ./campuscompute limits
    ./campuscompute demo
