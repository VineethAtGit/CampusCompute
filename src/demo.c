#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdio.h>

#include "demo.h"
#include "ipc.h"
#include "scheduler.h"
#include "banker.h"
#include "logger.h"
#include "worker.h"
#include "res_limits.h"

#define JOBS_FILE           "tests/jobs.txt"
#define BAD_JOBS_FILE       "tests/invalid_jobs.txt"
#define BANKER_SAFE_FILE    "tests/banker_safe.txt" 
#define BANKER_BAD_FILE     "tests/banker_unsafe.txt"

typedef int (*EntryFn)(int, char **);

typedef struct {

    const char  *title;
    EntryFn     fn;
    int         argc;
    char        *argv[4];
    int         expected;
    int         actual;
} Step;

static int check_leftovers(void) {

    int leaks = 0;

    if(waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD) {
        printf("    verified: no child process still left (no zombies)\n");
    } else {
        printf("    LEAK: a child process still exists\n");
        leaks++;
    }

    int fd = shm_open("/campus_shm", O_RDONLY, 0);
    if(fd >= 0) {
        close(fd);
        printf("    LEAK: /campus_shm exists\n");
        leaks++;
    } else {
        printf("    verified: shared memory /campus_shm is good\n");
    }

    sem_t *s1 = sem_open("/campus_sem_empty", 0);
    sem_t *s2 = sem_open("/campus_sem_full", 0);
    if(s1 != SEM_FAILED || s2 != SEM_FAILED) {
        if (s1 != SEM_FAILED) sem_close(s1);
        if (s2 != SEM_FAILED) sem_close(s2);
        printf("  LEAK: a named semaphore still exists\n");
        leaks++;
    } else {
        printf("    verified: named semaphores are gone\n");
    }

    return leaks;
}

int run_demo(int argc, char** argv) {
    if(argc > 1) {
        fprintf(stderr, "Error: 'demo' takes no arguements (got '%s)\n"
                        "Usage: campuscompute demo\n", argv[1]);

        return 2;
    }

    Step steps[] = {
        { "Scheduling: FCFS",              run_sched,  3, { "sched", "fcfs", JOBS_FILE },    0, 0 },
        { "Scheduling: Round Robin",       run_sched,  3, { "sched", "rr",   JOBS_FILE },    0, 0 },
        { "Scheduling: Priority",          run_sched,  3, { "sched", "prio", JOBS_FILE },    0, 0 },
        { "Scheduling: comparison",        run_sched,  3, { "sched", "compare", JOBS_FILE }, 0, 0 },
        { "Concurrent logger",             run_logger, 1, { "logger" },                      0, 0 },
        { "Shared-memory IPC",             run_ipc,    1, { "ipc" },                         0, 0 },
        { "Banker's algorithm (safe)",     run_banker, 2, { "banker", BANKER_SAFE_FILE },    0, 0 },
        { "Banker's algorithm (unsafe)",   run_banker, 2, { "banker", BANKER_BAD_FILE },     0, 0 },
        { "Worker: normal completion",     run_worker, 2, { "worker", "ok" },                0, 0 },
        { "Worker: simulated failure",     run_worker, 2, { "worker", "fail" },              0, 0 },
        { "Resource limits",               run_limits, 1, { "limits" },                      0, 0 },
        /* bad input must be rejected with usage code 2 */
        { "Invalid input is rejected",     run_sched,  3, { "sched", "fcfs", BAD_JOBS_FILE },2, 0 },
    };

    int n = (int)(sizeof steps / sizeof steps[0]);

    printf("##########  CampusCompute integrated demonstration  ##########\n");
    for(int i = 0; i < n; i++) {
        printf("\n---------- Step %d/%d: %s ----------\n", i + 1, n, steps[i].title);
        fflush(stdout);
        steps[i].actual = steps[i].fn(steps[i].argc, steps[i].argv);
        fflush(stdout);
    }

    printf("\n---------- Final cleanup check ----------\n");
    int leaks = check_leftovers();

    int passed = 0;
    printf("\n##########  SUMMARY  ##########\n");
    for (int i = 0; i < n; i++) {
        int ok = (steps[i].actual == steps[i].expected);
        passed += ok;
        printf("  %2d. %-30s %s (returned %d, expected %d)\n",
               i + 1, steps[i].title, ok ? "PASS" : "FAIL",
               steps[i].actual, steps[i].expected);
    }
    printf("  Cleanup (processes + IPC)       %s\n", leaks == 0 ? "PASS" : "FAIL");
    printf("\n  %d of %d steps passed.\n", passed, n);
 
    int all_ok = (passed == n && leaks == 0);
    printf("  Overall: %s\n", all_ok ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    return all_ok ? 0 : 1;
}