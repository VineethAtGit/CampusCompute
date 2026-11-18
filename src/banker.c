#include <stdio.h>
#include "banker.h"

int bankerSafety(int processes, int resources,
                 int available[],
                 int allocation[MAX_P][MAX_R],
                 int maximum[MAX_P][MAX_R])
{
    if (processes <= 0 || processes > MAX_P ||
        resources <= 0 || resources > MAX_R)
    {
        printf("Invalid number of processes or resources.\n");
        return 0;
    }

    int need[MAX_P][MAX_R];
    int finish[MAX_P] = {0};
    int safeSequence[MAX_P];
    int work[MAX_R];

    // Calculate Need = Maximum - Allocation
    for (int i = 0; i < processes; i++)
    {
        for (int j = 0; j < resources; j++)
        {
            if (allocation[i][j] < 0 ||
                maximum[i][j] < allocation[i][j])
            {
                printf("Invalid allocation or maximum value.\n");
                return 0;
            }

            need[i][j] = maximum[i][j] - allocation[i][j];
        }
    }

    for (int j = 0; j < resources; j++)
    {
        if (available[j] < 0)
        {
            printf("Invalid available resource value.\n");
            return 0;
        }

        work[j] = available[j];
    }

    printf("\nNeed Matrix:\n");

    for (int i = 0; i < processes; i++)
    {
        printf("P%d: ", i);

        for (int j = 0; j < resources; j++)
            printf("%d ", need[i][j]);

        printf("\n");
    }

    int count = 0;

    // Find a safe execution sequence
    while (count < processes)
    {
        int found = 0;

        for (int i = 0; i < processes; i++)
        {
            if (finish[i] == 0)
            {
                int possible = 1;

                for (int j = 0; j < resources; j++)
                {
                    if (need[i][j] > work[j])
                    {
                        possible = 0;
                        break;
                    }
                }

                if (possible)
                {
                    for (int j = 0; j < resources; j++)
                        work[j] += allocation[i][j];

                    safeSequence[count] = i;
                    count++;

                    finish[i] = 1;
                    found = 1;
                }
            }
        }

        if (!found)
            break;
    }

    if (count == processes)
    {
        printf("\nSystem is in a SAFE state.\n");
        printf("Safe Sequence: ");

        for (int i = 0; i < processes; i++)
        {
            printf("P%d", safeSequence[i]);

            if (i != processes - 1)
                printf(" -> ");
        }

        printf("\n");
        return 1;
    }
    else
    {
        printf("\nSystem is in an UNSAFE state.\n");
        printf("No safe sequence exists.\n");
        return 0;
    }
}
