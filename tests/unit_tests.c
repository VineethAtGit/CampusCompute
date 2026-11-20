/* Unit tests for job.c, scheduler.c and banker.c.  Run with: make unit */
#include <stdio.h>
#include <string.h>

#include "job.h"
#include "scheduler.h"
#include "banker.h"

static int run = 0, failed = 0;

#define CHECK(cond, name) do {                                   \
    run++;                                                       \
    if (cond) printf("  PASS  %s\n", name);                      \
    else { failed++; printf("  FAIL  %s  (line %d)\n", name, __LINE__); } \
} while (0)

static Job mk(int id, int arr, int burst, int prio)
{
    Job j;
    memset(&j, 0, sizeof j);
    j.job_id = id; j.arrival_time = arr; j.burst_time = burst; j.priority = prio;
    return j;
}

static void test_validate(void)
{
    printf("[validateJob]\n");
    CHECK(validateJob(mk(1, 0, 5, 0)) == 1,  "valid job accepted");
    CHECK(validateJob(mk(0, 0, 5, 0)) == 0,  "id 0 rejected");
    CHECK(validateJob(mk(-1, 0, 5, 0)) == 0, "negative id rejected");
    CHECK(validateJob(mk(1, -1, 5, 0)) == 0, "negative arrival rejected");
    CHECK(validateJob(mk(1, 0, 0, 0)) == 0,  "zero burst rejected");
    CHECK(validateJob(mk(1, 0, -5, 0)) == 0, "negative burst rejected");
    CHECK(validateJob(mk(1, 0, 5, -1)) == 0, "negative priority rejected");
}

static void test_job_list(void)
{
    printf("[job list: add / find / update / remove]\n");
    Job jobs[MAX_JOBS];
    int n = 0;

    CHECK(addJob(jobs, &n, mk(1, 0, 5, 1)) == 1 && n == 1, "add first job");
    CHECK(addJob(jobs, &n, mk(2, 1, 3, 2)) == 1 && n == 2, "add second job");
    CHECK(addJob(jobs, &n, mk(3, 2, 4, 3)) == 1 && n == 3, "add third job");
    CHECK(addJob(jobs, &n, mk(9, 0, 0, 0)) == 0 && n == 3, "invalid job not added");

    Job *f = findJob(jobs, n, 2);
    CHECK(f != NULL && f->job_id == 2, "findJob returns existing job");
    CHECK(findJob(jobs, n, 42) == NULL, "findJob returns NULL for unknown id");

    CHECK(updateJobStatus(jobs, n, 2, RUNNING) == 1 && jobs[1].status == RUNNING,
          "updateJobStatus changes status");
    CHECK(updateJobStatus(jobs, n, 2, 99) == 0, "invalid status rejected");
    CHECK(updateJobStatus(jobs, n, 42, READY) == 0, "unknown id rejected");

    /* regression test: removeJob used to compare an int with a pointer */
    CHECK(removeJob(jobs, &n, 1) == 1 && n == 2, "remove first job");
    CHECK(jobs[0].job_id == 2 && jobs[1].job_id == 3, "remaining jobs shifted left");
    CHECK(removeJob(jobs, &n, 3) == 1 && n == 1, "remove last job");
    CHECK(removeJob(jobs, &n, 3) == 0 && n == 1, "removing missing job fails");
    CHECK(removeJob(jobs, &n, 2) == 1 && n == 0, "remove only job");

    n = MAX_JOBS;
    CHECK(addJob(jobs, &n, mk(1, 0, 1, 0)) == 0, "addJob refuses when list is full");
}

static void test_initialize(void)
{
    printf("[initializeJobs]\n");
    Job jobs[2] = { mk(1, 0, 6, 1), mk(2, 4, 3, 1) };
    jobs[0].completion_time = 99;           /* must be reset */
    initializeJobs(jobs, 2);
    CHECK(jobs[0].remaining_time == 6 && jobs[1].remaining_time == 3, "remaining = burst");
    CHECK(jobs[0].completion_time == 0, "completion time reset");
    CHECK(jobs[0].status == READY && jobs[1].status == WAITING,
          "arrival 0 -> READY, later -> WAITING");
}

static void test_sort(void)
{
    printf("[sortJobsByArrival]\n");
    Job jobs[4] = { mk(4, 5, 1, 0), mk(3, 0, 1, 0), mk(1, 0, 1, 0), mk(2, 2, 1, 0) };
    sortJobsByArrival(jobs, 4);
    CHECK(jobs[0].job_id == 1 && jobs[1].job_id == 3, "equal arrival ordered by lower id");
    CHECK(jobs[2].job_id == 2 && jobs[3].job_id == 4, "later arrivals follow in order");
}

static void test_banker(void)
{
    printf("[bankerSafety]\n");
    int avail_safe[MAX_R] = {3, 3, 2};
    int avail_bad[MAX_R]  = {0, 0, 0};
    int alloc[MAX_P][MAX_R] = {{0,1,0},{2,0,0},{3,0,2},{2,1,1},{0,0,2}};
    int max[MAX_P][MAX_R]   = {{7,5,3},{3,2,2},{9,0,2},{2,2,2},{4,3,3}};

    CHECK(bankerSafety(5, 3, avail_safe, alloc, max) == 1, "textbook example is SAFE");
    CHECK(bankerSafety(5, 3, avail_bad, alloc, max) == 0,  "no free resources is UNSAFE");

    int bad_max[MAX_P][MAX_R] = {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
    CHECK(bankerSafety(5, 3, avail_safe, alloc, bad_max) == 0, "max < allocation rejected");

    int neg_avail[MAX_R] = {-1, 0, 0};
    CHECK(bankerSafety(5, 3, neg_avail, alloc, max) == 0, "negative available rejected");

    CHECK(bankerSafety(0, 3, avail_safe, alloc, max) == 0, "zero processes rejected");
    CHECK(bankerSafety(MAX_P + 1, 3, avail_safe, alloc, max) == 0, "too many processes rejected");
    CHECK(bankerSafety(5, 0, avail_safe, alloc, max) == 0, "zero resources rejected");
}

int main(void)
{
    printf("=== CampusCompute unit tests ===\n");
    test_validate();
    test_job_list();
    test_initialize();
    test_sort();
    test_banker();
    printf("\n%d checks, %d failed -> %s\n", run, failed, failed ? "FAIL" : "ALL PASSED");
    return failed ? 1 : 0;
}
