
#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"

#define MAX_JOBS 100

// Sort jobs according to arrival time
void sortJobsByArrival(Job jobs[], int n)
{
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - i - 1; j++)
        {
            if (jobs[j].arrival_time > jobs[j + 1].arrival_time ||
                (jobs[j].arrival_time == jobs[j + 1].arrival_time &&
                 jobs[j].job_id > jobs[j + 1].job_id))
            {
                Job temp = jobs[j];
                jobs[j] = jobs[j + 1];
                jobs[j + 1] = temp;
            }
        }
    }
}

// Display scheduling results
void displayResults(Job jobs[], int n, float totalTAT, float totalWT)
{
    printf("\nJob\tCT\tTAT\tWT\n");

    for (int i = 0; i < n; i++)
    {
        printf("J%d\t%d\t%d\t%d\n",
               jobs[i].job_id,
               jobs[i].completion_time,
               jobs[i].turnaround_time,
               jobs[i].waiting_time);
    }

    printf("\nAverage Turnaround Time: %.2f\n", totalTAT / n);
    printf("Average Waiting Time: %.2f\n", totalWT / n);
}

// FCFS Scheduling
void fcfsScheduling(Job jobs[], int n)
{
    if (n <= 0 || n > MAX_JOBS)
    {
        printf("Invalid number of jobs.\n");
        return;
    }

    Job copyofjobs[MAX_JOBS];

    for (int i = 0; i < n; i++)
        copyofjobs[i] = jobs[i];

    sortJobsByArrival(copyofjobs, n);

    int cpuTime = 0;
    float totalTAT = 0, totalWT = 0;

    printf("\n--- FCFS Scheduling ---\n");
    printf("Timeline:\n");

    for (int i = 0; i < n; i++)
    {
        if (cpuTime < copyofjobs[i].arrival_time)
            cpuTime = copyofjobs[i].arrival_time;

        int start = cpuTime;
        cpuTime += copyofjobs[i].burst_time;

        copyofjobs[i].completion_time = cpuTime;
        copyofjobs[i].turnaround_time =
            cpuTime - copyofjobs[i].arrival_time;
        copyofjobs[i].waiting_time =
            copyofjobs[i].turnaround_time -
            copyofjobs[i].burst_time;

        copyofjobs[i].remaining_time = 0;
        copyofjobs[i].status = COMPLETED;

        printf("J%d (%d-%d)\n",
               copyofjobs[i].job_id, start, cpuTime);

        totalTAT += copyofjobs[i].turnaround_time;
        totalWT += copyofjobs[i].waiting_time;
    }

    displayResults(copyofjobs, n, totalTAT, totalWT);
}

// Round Robin Scheduling
void roundRobinScheduling(Job jobs[], int n, int quantum)
{
    if (n <= 0 || n > MAX_JOBS || quantum <= 0)
    {
        printf("Invalid number of jobs or time quantum.\n");
        return;
    }

    Job copyofjobs[MAX_JOBS];

    long long totalBurst = 0;

    for (int i = 0; i < n; i++)
    {
        if (jobs[i].burst_time <= 0)
        {
            printf("Invalid burst time.\n");
            return;
        }

        copyofjobs[i] = jobs[i];
        copyofjobs[i].remaining_time = jobs[i].burst_time;
        copyofjobs[i].completion_time = 0;

        totalBurst += jobs[i].burst_time;
    }

    // Maximum possible number of queue insertions
    size_t queueSize = (size_t)totalBurst + (size_t)n;

    if (queueSize > (size_t)-1 / sizeof(int))
    {
        printf("Queue size is too large.\n");
        return;
    }

    // Dynamically allocate the queue
    int *queue = malloc(queueSize * sizeof(int));

    if (queue == NULL)
    {
        printf("Memory allocation failed.\n");
        return;
    }

    int head = 0, tail = 0;
    int cpuTime = 0;
    int completed = 0;
    int nextJob = 0;

    float totalTAT = 0, totalWT = 0;

    sortJobsByArrival(copyofjobs, n);

    printf("\n--- Round Robin Scheduling ---\n");
    printf("Time Quantum: %d\n", quantum);
    printf("Timeline:\n");

    while (completed < n)
    {
        // Add arrived jobs to the queue
        while (nextJob < n &&
               copyofjobs[nextJob].arrival_time <= cpuTime)
        {
            queue[tail++] = nextJob;
            nextJob++;
        }

        // If queue is empty, move to next arrival
        if (head == tail)
        {
            if (nextJob < n)
            {
                cpuTime = copyofjobs[nextJob].arrival_time;
                continue;
            }
        }

        int index = queue[head++];
        int start = cpuTime;

        int runTime = copyofjobs[index].remaining_time;

        if (runTime > quantum)
            runTime = quantum;

        cpuTime += runTime;
        copyofjobs[index].remaining_time -= runTime;

        printf("J%d (%d-%d)\n",
               copyofjobs[index].job_id, start, cpuTime);

        // Add newly arrived jobs
        while (nextJob < n &&
               copyofjobs[nextJob].arrival_time <= cpuTime)
        {
            queue[tail++] = nextJob;
            nextJob++;
        }

        if (copyofjobs[index].remaining_time == 0)
        {
            copyofjobs[index].completion_time = cpuTime;
            copyofjobs[index].turnaround_time =
                cpuTime - copyofjobs[index].arrival_time;
            copyofjobs[index].waiting_time =
                copyofjobs[index].turnaround_time -
                copyofjobs[index].burst_time;

            copyofjobs[index].status = COMPLETED;

            totalTAT += copyofjobs[index].turnaround_time;
            totalWT += copyofjobs[index].waiting_time;

            completed++;
        }
        else
        {
            // Put unfinished job back into queue
            queue[tail++] = index;
        }
    }

    displayResults(copyofjobs, n, totalTAT, totalWT);

    // Release allocated memory
    free(queue);
}

// Non-preemptive Priority Scheduling
// Smaller priority number means higher priority
void priorityScheduling(Job jobs[], int n)
{
    if (n <= 0 || n > MAX_JOBS)
    {
        printf("Invalid number of jobs.\n");
        return;
    }

    Job copyofjobs[MAX_JOBS];
    int completed[MAX_JOBS] = {0};

    for (int i = 0; i < n; i++)
        copyofjobs[i] = jobs[i];

    int cpuTime = 0;
    int count = 0;

    float totalTAT = 0, totalWT = 0;

    printf("\n--- Priority Scheduling ---\n");
    printf("Smaller priority number = Higher priority\n");
    printf("Timeline:\n");

    while (count < n)
    {
        int selected = -1;

        for (int i = 0; i < n; i++)
        {
            if (!completed[i] &&
                copyofjobs[i].arrival_time <= cpuTime)
            {
                if (selected == -1 ||
                    copyofjobs[i].priority < copyofjobs[selected].priority ||
                    (copyofjobs[i].priority == copyofjobs[selected].priority &&
                     copyofjobs[i].arrival_time < copyofjobs[selected].arrival_time) ||
                    (copyofjobs[i].priority == copyofjobs[selected].priority &&
                     copyofjobs[i].arrival_time == copyofjobs[selected].arrival_time &&
                     copyofjobs[i].job_id < copyofjobs[selected].job_id))
                {
                    selected = i;
                }
            }
        }

        if (selected == -1)
        {
            cpuTime++;
            continue;
        }

        int start = cpuTime;
        cpuTime += copyofjobs[selected].burst_time;

        copyofjobs[selected].completion_time = cpuTime;
        copyofjobs[selected].turnaround_time =
            cpuTime - copyofjobs[selected].arrival_time;
        copyofjobs[selected].waiting_time =
            copyofjobs[selected].turnaround_time -
            copyofjobs[selected].burst_time;

        copyofjobs[selected].status = COMPLETED;

        printf("J%d (%d-%d)\n",
               copyofjobs[selected].job_id, start, cpuTime);

        totalTAT += copyofjobs[selected].turnaround_time;
        totalWT += copyofjobs[selected].waiting_time;

        completed[selected] = 1;
        count++;
    }

    displayResults(copyofjobs, n, totalTAT, totalWT);
}
