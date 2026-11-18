#ifndef CC_BANKER_H
#define CC_BANKER_H
#define MAX_P 10
#define MAX_R 10
int bankerSafety(int processes, int resources, int available[],
                 int allocation[MAX_P][MAX_R], int maximum[MAX_P][MAX_R]);
#endif
