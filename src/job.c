#include <stdio.h>
#include <stdlib.h>
#include "C:\Users\Lenovo\Desktop\Sem 3\os\os project\CampusCompute\include\job.h"
#define MAX_JOBS 100

int main(){
    return 0;
}

void initializeJobs(Job jobs[], int n){
    for(int i=0; i<n ; i++){
        jobs[i].remaining_time = jobs[i].burst_time;
        jobs[i].completion_time = 0;
        jobs[i].turnaround_time = 0;
        jobs[i].waiting_time = 0;

        if (jobs[i].arrival_time == 0) jobs[i].status = READY;
        else jobs[i].status = WAITING;
    }
}

int validateJob(Job job){
    if(job.job_id <= 0) return 0;
    else if(job.arrival_time < 0) return 0;
    else if(job.burst_time <= 0) return 0;
    else if(job.priority < 0) return 0;
    else return 1;
}

void displayJobs(Job jobs[], int n){
    printf("%-8s %-8s %-6s %-10s %-8s\n",
       "Job ID", "Arrival", "Burst", "Priority", "Status");

    for(int i=0 ; i<n ; i++){
        char *status;
        if(jobs[i].status == WAITING)
            status = "WAITING";
        else if(jobs[i].status == READY)
            status = "READY";
        else if(jobs[i].status == RUNNING)
            status = "RUNNING";
        else
            status = "COMPLETED";

        printf("%-8d %-8d %-6d %-10d %-8s\n",
            jobs[i].job_id,
            jobs[i].arrival_time,
            jobs[i].burst_time,
            jobs[i].priority,
            status);
    }
}

int addJob(Job jobs[], int *n, Job newJob){
    if(*n >= MAX_JOBS)
        return 0;
    else if(validateJob(newJob) == 1){
        jobs[*n] = newJob;
        (*n)++;
        return 1;
    }
    else 
        return 0;
}

Job *findJob(Job jobs[], int n, int jobId){
    for(int i = 0 ; i < n ; i++){
        if(jobs[i].job_id == jobId)
            return &jobs[i];
    }
    return NULL;
}

int updateJobStatus(Job jobs[], int n, int jobId, int newStatus){
    Job *job = findJob(jobs, n, jobId);
    if(job == NULL)
        return 0;
    else if(newStatus != WAITING &&
            newStatus != READY &&
            newStatus != RUNNING &&
            newStatus != COMPLETED)
        return 0;
    else{
        job->status = newStatus;
        return 1;
    }
}

int removeJob(Job jobs[], int *n, int jobId){
    Job *job = findJob(jobs, *n, jobId);
    if(job == NULL)
        return 0;
    int index = job - jobs;
    for(int i = index; i < n-1; i++){
        jobs[i] = jobs[i + 1];
    }
    (*n)--;
    return 1;
}