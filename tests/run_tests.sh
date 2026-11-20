#!/usr/bin/env bash
# Command-line tests for CampusCompute. Run from the project root: make cli
BIN=./campuscompute
[ -x "$BIN" ] || { echo "Build first: make"; exit 1; }

pass=0; fail=0; OUT=""; RC=0

run() { OUT=$("$@" 2>&1); RC=$?; }           # capture stdout+stderr and exit code

ok()   { pass=$((pass+1)); printf "  PASS  %s\n" "$1"; }
bad()  { fail=$((fail+1)); printf "  FAIL  %s\n" "$1"; printf '        --- output ---\n%s\n        ---------------\n' "$OUT" | head -25; }
has()  { grep -qE -- "$1" <<<"$OUT"; }

# expect <name> <expected rc> <regex that must appear> [regex that must NOT appear]
expect() {
    local name=$1 rc=$2 must=$3 mustnot=$4
    if [ "$RC" -ne "$rc" ]; then bad "$name (exit $RC, expected $rc)"; return; fi
    if [ -n "$must" ] && ! has "$must"; then bad "$name (missing: $must)"; return; fi
    if [ -n "$mustnot" ] && has "$mustnot"; then bad "$name (unexpected: $mustnot)"; return; fi
    ok "$name"
}

echo "=== CampusCompute command-line tests ==="

echo "[CLI]"
run $BIN help;            expect "help prints usage"            0 "usage"
run $BIN;                 expect "no arguments -> usage, exit 1" 1 "usage"
run $BIN bogus;           expect "unknown command -> exit 2"    2 "unknown command"

echo "[Scheduling: expected timelines and averages for tests/jobs.txt]"
run $BIN sched fcfs tests/jobs.txt
expect "FCFS timeline"  0 "J1 \(0-8\)"
expect "FCFS order"     0 "J4 \(21-26\)"
expect "FCFS averages"  0 "Average Turnaround Time: 15.25" ; has "Average Waiting Time: 8.75" && ok "FCFS waiting avg" || bad "FCFS waiting avg"
run $BIN sched rr tests/jobs.txt
expect "RR quantum 4, first slice"   0 "J1 \(0-4\)"
expect "RR preemption (J1 resumes)"  0 "J1 \(16-20\)"
expect "RR averages"                 0 "Average Turnaround Time: 18.25"; has "Average Waiting Time: 11.75" && ok "RR waiting avg" || bad "RR waiting avg"
run $BIN sched prio tests/jobs.txt
expect "Priority picks J4 before J3" 0 "J4 \(12-17\)"
expect "Priority averages"           0 "Average Turnaround Time: 14.25"; has "Average Waiting Time: 7.75" && ok "Priority waiting avg" || bad "Priority waiting avg"
run $BIN sched compare tests/jobs.txt
expect "compare shows all three"     0 "FCFS Scheduling" ; has "Round Robin Scheduling" && has "Priority Scheduling" && ok "compare has RR + Priority" || bad "compare has RR + Priority"
expect "all jobs end COMPLETED"      0 "COMPLETED"

echo "[Scheduling: edge cases]"
run $BIN sched fcfs tests/jobs_tie.txt
expect "tie on arrival -> lower id first" 0 "J1 \(0-3\)"
run $BIN sched fcfs tests/jobs_idle.txt
expect "idle gap: job 2 starts at its arrival" 0 "J2 \(10-13\)"
run $BIN sched rr tests/jobs_idle.txt
expect "RR handles idle gap" 0 "J2 \(10-13\)"
run $BIN sched prio tests/jobs_idle.txt
expect "Priority handles idle gap" 0 "J2 \(10-13\)"

echo "[Scheduling: invalid input is rejected with exit code 2]"
run $BIN sched fcfs tests/invalid_jobs.txt;  expect "negative burst / non-numeric rejected" 2 "Error"
run $BIN sched fcfs tests/jobs_duplicate.txt; expect "duplicate job id rejected"           2 "duplicate"
run $BIN sched fcfs tests/jobs_empty.txt;     expect "file with no jobs rejected"          2 "no jobs"
run $BIN sched fcfs tests/does_not_exist.txt; expect "missing file rejected"               2 "cannot open"
run $BIN sched xyz tests/jobs.txt;            expect "unknown algorithm rejected"          2 "unknown algorithm"
run $BIN sched fcfs;                          expect "missing file argument rejected"      2 "Usage"

echo "[Banker's algorithm]"
run $BIN banker tests/banker_safe.txt
expect "safe state detected" 0 "SAFE state" "UNSAFE"
expect "safe sequence printed" 0 "P1 -> P3 -> P4 -> P0 -> P2"
run $BIN banker tests/banker_unsafe.txt;  expect "unsafe state detected (exit 0)" 0 "UNSAFE"
run $BIN banker tests/banker_invalid.txt; expect "invalid state file rejected"    2 "Error"
run $BIN banker tests/nope.txt;           expect "missing banker file rejected"   2 "cannot open"
run $BIN banker;                          expect "banker without file -> usage"   2 "Usage|usage"

echo "[Worker (fork / waitpid)]"
run $BIN worker ok;   expect "worker ok finishes normally" 0 "finished normally|completed successfully"
run $BIN worker fail; expect "worker failure is detected, exit 0" 0 "exit code 1|simulated"
run $BIN worker xyz;  expect "bad worker argument rejected" 2 "Error|Usage|usage"

echo "[Logger (pthreads)]"
rm -f logs/campus.log
run $BIN logger
expect "logger passes" 0 "Result: PASS"
expect "15 events produced and written" 0 "produced: 15, events written to log: 15"
if [ -f logs/campus.log ] && [ "$(grep -c '\[P' logs/campus.log)" -eq 15 ]; then ok "log file has 15 entries"; else bad "log file has 15 entries"; fi
run $BIN logger extra;   expect "logger rejects arguments" 2 "takes no arguments"

echo "[Shared-memory IPC]"
run $BIN ipc
expect "ipc passes, sum is 150" 0 "sum from shared memory = 150"
expect "ipc cleanup verified"   0 "verified: /campus_shm removed" "LEAK"
if ls /dev/shm 2>/dev/null | grep -q campus; then bad "no /dev/shm leftovers"; else ok "no /dev/shm leftovers"; fi
run $BIN ipc extra;      expect "ipc rejects arguments" 2 "takes no arguments"

echo "[Resource limits]"
run $BIN limits
expect "memory limit enforced"   0 "300 MB request: DENIED"
expect "CPU limit sends SIGXCPU" 0 "killed by signal 24|CPU time limit"
expect "limits passes"           0 "Result: PASS"

echo "[Integrated demo]"
run $BIN demo
expect "demo: all steps pass" 0 "ALL TESTS PASSED" "FAIL"
expect "demo: 12 of 12"       0 "12 of 12 steps passed"
run $BIN demo extra; expect "demo rejects arguments" 2 "takes no arg"

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ] && echo "ALL CLI TESTS PASSED" || echo "SOME CLI TESTS FAILED"
[ "$fail" -eq 0 ]
