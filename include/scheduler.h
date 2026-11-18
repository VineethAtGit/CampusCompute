#ifndef CC_SCHEDULER_H
#define CC_SCHEDULER_H

#include "job.h"

void sortJobsByArrival(Job jobs[], int n);
void fcfsScheduling(Job jobs[], int n);
void roundRobinScheduling(Job jobs[], int n, int quantum);
void priorityScheduling(Job jobs[], int n);

#endif
