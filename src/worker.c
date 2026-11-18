
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "worker.h"

void runWorker(int simulateFailure)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
        return;
    }

    // Child process
    if (pid == 0)
    {
        printf("\nChild process started. PID: %d\n", getpid());

        if (simulateFailure)
        {
            printf("Simulating worker failure...\n");
            exit(1);
        }
        else
        {
            printf("Worker completed successfully.\n");
            exit(0);
        }
    }

    // Parent process
    else
    {
        int status;

        printf("Parent waiting for child process...\n");

        if (waitpid(pid, &status, 0) == -1)
        {
            perror("waitpid failed");
            return;
        }

        if (WIFEXITED(status))
        {
            int exitCode = WEXITSTATUS(status);

            if (exitCode == 0)
                printf("Worker finished normally.\n");
            else
                printf("Worker failed with exit code %d (simulated failure).\n",
                       exitCode);
        }
        else if (WIFSIGNALED(status))
        {
            printf("Worker terminated by signal %d.\n",
                   WTERMSIG(status));
        }
    }
}
