/* Adapters: connect CLI commands to the function-style modules written by
 * the teammate (job.c, scheduler.c, banker.c, worker.c).  (Owner: You)
 *
 * Each run_* function:
 *   1. checks the command-line arguments,
 *   2. reads and validates the input file (the teammate's functions take
 *      arrays, not file names),
 *   3. calls the teammate's function,
 *   4. returns 0 on success, 1 on runtime error, 2 on bad usage or bad input.
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adapters.h"
#include "job.h"
#include "scheduler.h"
#include "banker.h"
#include "worker.h"

#ifndef MAX_JOBS
#define MAX_JOBS 100
#endif
#define RR_QUANTUM 4

/* ================= jobs file ================= */

/* Format: one job per line "id arrival burst priority"; '#' starts a comment.
 * Returns the number of jobs, or -1 after printing an error. */
static int load_jobs_file(const char *path, Job jobs[], int max)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s': %s\n", path, strerror(errno));
        return -1;
    }

    char line[256];
    int n = 0, lineno = 0, rc = 0;

    while (rc == 0 && fgets(line, sizeof line, f)) {
        lineno++;
        if (!strchr(line, '\n') && !feof(f)) {
            fprintf(stderr, "Error: line %d is too long\n", lineno);
            rc = -1;
            break;
        }
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '#' || *p == '\0') continue;          /* comment / blank */

        Job j;
        memset(&j, 0, sizeof j);
        int used = 0;
        if (sscanf(p, "%d %d %d %d %n", &j.job_id, &j.arrival_time,
                   &j.burst_time, &j.priority, &used) != 4) {
            fprintf(stderr, "Error: line %d: expected 4 integers "
                            "(id arrival burst priority)\n", lineno);
            rc = -1;
        } else if (p[used] != '\0' && p[used] != '#') {
            fprintf(stderr, "Error: line %d: unexpected extra text\n", lineno);
            rc = -1;
        } else if (!validateJob(j)) {
            fprintf(stderr, "Error: line %d: invalid job "
                            "(need id>0, arrival>=0, burst>0, priority>=0)\n", lineno);
            rc = -1;
        } else if (findJob(jobs, n, j.job_id) != NULL) {
            fprintf(stderr, "Error: line %d: duplicate job id %d\n", lineno, j.job_id);
            rc = -1;
        } else if (n >= max) {
            fprintf(stderr, "Error: more than %d jobs\n", max);
            rc = -1;
        } else {
            jobs[n++] = j;
        }
    }
    fclose(f);

    if (rc == 0 && n == 0) {
        fprintf(stderr, "Error: '%s' contains no jobs\n", path);
        rc = -1;
    }
    if (rc != 0) return -1;

    initializeJobs(jobs, n);
    return n;
}

static void sched_usage(void)
{
    fprintf(stderr, "Usage: campuscompute sched fcfs|rr|prio|compare <jobs file>\n");
}

int run_sched(int argc, char **argv)
{
    if (argc != 3) { sched_usage(); return 2; }

    const char *algo = argv[1];
    if (strcmp(algo, "fcfs") && strcmp(algo, "rr") &&
        strcmp(algo, "prio") && strcmp(algo, "compare")) {
        fprintf(stderr, "Error: unknown algorithm '%s'\n", algo);
        sched_usage();
        return 2;
    }

    Job jobs[MAX_JOBS];
    int n = load_jobs_file(argv[2], jobs, MAX_JOBS);
    if (n < 0) return 2;

    printf("=== CPU scheduling (%s) on %s: %d jobs ===\n\n", algo, argv[2], n);
    printf("Loaded jobs:\n");
    displayJobs(jobs, n);
    printf("\nTie-breaking: equal arrival times are ordered by lower job ID; "
           "Priority picks the smaller priority number, then earlier arrival, "
           "then lower job ID.\n");
    printf("Round Robin time quantum: %d\n", RR_QUANTUM);

    if (!strcmp(algo, "fcfs") || !strcmp(algo, "compare"))
        fcfsScheduling(jobs, n);
    if (!strcmp(algo, "rr") || !strcmp(algo, "compare"))
        roundRobinScheduling(jobs, n, RR_QUANTUM);
    if (!strcmp(algo, "prio") || !strcmp(algo, "compare"))
        priorityScheduling(jobs, n);

    /* lifecycle: every job has now run to completion */
    for (int i = 0; i < n; i++)
        updateJobStatus(jobs, n, jobs[i].job_id, COMPLETED);
    printf("\nJob states after scheduling:\n");
    displayJobs(jobs, n);
    return 0;
}

/* ================= Banker's file ================= */

/* Format: first data line "P R", then Available (R numbers), then Allocation
 * (P rows of R), then Maximum (P rows of R). '#' starts a comment. */
#define BANKER_MAX_VALUES (2 + MAX_R + 2 * MAX_P * MAX_R)

static int load_banker_file(const char *path, int *P, int *R, int available[],
                            int allocation[MAX_P][MAX_R], int maximum[MAX_P][MAX_R])
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s': %s\n", path, strerror(errno));
        return -1;
    }

    int vals[BANKER_MAX_VALUES];
    int count = 0, lineno = 0, rc = 0;
    char line[512];

    while (rc == 0 && fgets(line, sizeof line, f)) {
        lineno++;
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        char *p = line;
        for (;;) {
            while (isspace((unsigned char)*p)) p++;
            if (*p == '\0') break;
            char *end;
            errno = 0;
            long v = strtol(p, &end, 10);
            if (end == p || errno != 0 || v < -1000000 || v > 1000000 ||
                (*end != '\0' && !isspace((unsigned char)*end))) {
                fprintf(stderr, "Error: line %d: '%.12s' is not a valid integer\n", lineno, p);
                rc = -1;
                break;
            }
            if (count >= BANKER_MAX_VALUES) {
                fprintf(stderr, "Error: line %d: too many values in file\n", lineno);
                rc = -1;
                break;
            }
            vals[count++] = (int)v;
            p = end;
        }
    }
    fclose(f);
    if (rc != 0) return -1;

    if (count < 2) {
        fprintf(stderr, "Error: file must start with 'P R' (processes, resource types)\n");
        return -1;
    }
    *P = vals[0];
    *R = vals[1];
    if (*P < 1 || *P > MAX_P || *R < 1 || *R > MAX_R) {
        fprintf(stderr, "Error: P must be 1..%d and R must be 1..%d (got %d and %d)\n",
                MAX_P, MAX_R, *P, *R);
        return -1;
    }
    int expected = 2 + *R + 2 * (*P) * (*R);
    if (count != expected) {
        fprintf(stderr, "Error: expected %d numbers for P=%d R=%d, found %d\n",
                expected, *P, *R, count);
        return -1;
    }

    int k = 2;
    for (int j = 0; j < *R; j++) available[j] = vals[k++];
    for (int i = 0; i < *P; i++) for (int j = 0; j < *R; j++) allocation[i][j] = vals[k++];
    for (int i = 0; i < *P; i++) for (int j = 0; j < *R; j++) maximum[i][j] = vals[k++];

    for (int j = 0; j < *R; j++)
        if (available[j] < 0) {
            fprintf(stderr, "Error: Available[%d] is negative\n", j);
            return -1;
        }
    for (int i = 0; i < *P; i++)
        for (int j = 0; j < *R; j++) {
            if (allocation[i][j] < 0 || maximum[i][j] < 0) {
                fprintf(stderr, "Error: negative value for P%d, resource %d\n", i, j);
                return -1;
            }
            if (maximum[i][j] < allocation[i][j]) {
                fprintf(stderr, "Error: P%d resource %d: Maximum (%d) is less than Allocation (%d)\n",
                        i, j, maximum[i][j], allocation[i][j]);
                return -1;
            }
        }
    return 0;
}

static void print_matrix(const char *title, int rows, int cols, int m[MAX_P][MAX_R])
{
    printf("%s:\n", title);
    for (int i = 0; i < rows; i++) {
        printf("  P%d:", i);
        for (int j = 0; j < cols; j++) printf(" %d", m[i][j]);
        printf("\n");
    }
}

int run_banker(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: campuscompute banker <state file>\n");
        return 2;
    }

    int P, R;
    int available[MAX_R];
    int allocation[MAX_P][MAX_R], maximum[MAX_P][MAX_R];
    if (load_banker_file(argv[1], &P, &R, available, allocation, maximum) != 0)
        return 2;

    printf("=== Banker's safety algorithm on %s (P=%d, R=%d) ===\n\n", argv[1], P, R);
    printf("Available:");
    for (int j = 0; j < R; j++) printf(" %d", available[j]);
    printf("\n");
    print_matrix("Allocation", P, R, allocation);
    print_matrix("Maximum", P, R, maximum);

    /* prints the Need matrix and the SAFE / UNSAFE verdict itself.
     * Both verdicts mean the check worked, so the command returns 0. */
    (void)bankerSafety(P, R, available, allocation, maximum);
    return 0;
}

/* ================= worker ================= */

int run_worker(int argc, char **argv)
{
    int fail;
    if (argc != 2 || (strcmp(argv[1], "ok") && strcmp(argv[1], "fail"))) {
        fprintf(stderr, "Usage: campuscompute worker ok|fail\n");
        return 2;
    }
    fail = (strcmp(argv[1], "fail") == 0);

    printf("=== Worker process demo (%s) ===\n", fail ? "simulated failure" : "normal completion");
    if (fail)
        printf("NOTE: this failure is SIMULATED by the program (the child "
               "deliberately exits with code 1); it is not a real OS failure.\n");
    fflush(stdout);                 /* do not let the child copy buffered output */

    runWorker(fail);
    return 0;
}
