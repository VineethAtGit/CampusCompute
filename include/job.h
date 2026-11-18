#ifndef CC_JOB_H
#define CC_JOB_H

#define MAX_JOBS 100

#define WAITING 0
#define READY 1
#define RUNNING 2
#define COMPLETED 3

typedef struct Job
{
    int job_id;
    int arrival_time;
    int burst_time;
    int priority;

    int remaining_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int status;

} Job;

void initializeJobs(Job jobs[], int n);
int validateJob(Job job);
void displayJobs(Job jobs[], int n);
int addJob(Job jobs[], int *n, Job newJob);
Job *findJob(Job jobs[], int n, int jobId);
int updateJobStatus(Job jobs[], int n, int jobId, int newStatus);
int removeJob(Job jobs[], int *n, int jobId);

#endif
