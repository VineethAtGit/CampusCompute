#ifndef CC_JOB_H
#define CC_JOB_H
#define MAX_JOBS 100

typedef enum { WAITING, READY, RUNNING, COMPLETED } JobStatus;

typedef struct {
    int job_id;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int priority;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    JobStatus status;
} Job;

void initializeJobs(Job jobs[], int n);
int  validateJob(Job job);
void displayJobs(Job jobs[], int n);
int  addJob(Job jobs[], int *n, Job newJob);
Job *findJob(Job jobs[], int n, int jobId);
int  updateJobStatus(Job jobs[], int n, int jobId, int newStatus);
int  removeJob(Job jobs[], int *n, int jobId);
#endif
