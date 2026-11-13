#include<errno.h>
#include <signal.h> // for signal related functions
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include "res_limits.h"

#define MB (1024UL * 1024UL) 

#define CHILD_LIMIT_ENFORCED 0
#define CHILD_LIMIT_IGNORED 4
#define CHILD_SETUP_FAILED 5

static void print_limit(const char* label, int resource) { 

    struct rlimit rl;

    // error handling : getrlimit
    if (getrlimit(resource, &rl) != 0) {
        fprintf(stderr, "Error : getrlimit failed : %s\n", stderror(errno));
        return;
    }

    // printing soft and hard resource limits
    printf("  %s: soft=", label); 
    if (rl.rlim_cur == RLIM_INFINITY) printf("unlimited");
    else printf("%lu", (unsigned long)rl.rlim_cur);
    printf(", hard=");
    if (rl.rlim_max == RLIM_INFINITY) printf("unlimited\n");
    else printf("%lu\n", (unsigned long)rl.rlim_max);
}

static int wait_for_child(pid_t pid, int * status) {
    while(waitpid(pid, status, 0) < 0) { 
        // error handling : waitpid
        if(errno != EINTR) { 
            fprintf(stderr, "Error: waitpid failed: %s\n", stderror(errno));
            return -1;
        }
    }

    return 0;
}

static void memory_child(void) { 

    struct rlimit rl = { 100 * MB, 100 * MB };   /* 100 MB address space */

    print_limit("after ", RLIMIT_AS); 

    printf("  [child] requesting 20 MB (inside the limit)...\n");
    void *small = malloc(20 * MB);
    printf("  [child] 20 MB request: %s\n", small ? "granted" : "DENIED");
    free(small);
 
    printf("  [child] requesting 300 MB (over the limit)...\n");
    void *big = malloc(300 * MB);
    printf("  [child] 300 MB request: %s\n", big ? "granted" : "DENIED (malloc returned NULL)");

    int code = big ? CHILD_LIMIT_IGNORED : CHILD_LIMIT_ENFORCED;
    free(big);
    fflush(stdout);
    _exit(code);
}

static int demo_memory_limit(void) {

    printf("\n[Demo 1] RLIMIT_AS: cap the child's memory at 100 MB\n");
    fflush(stdout);                 /* avoid duplicated buffered output after fork */

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Error: fork failed: %s\n", strerror(errno));
        return 1;
    }
    if (pid == 0) memory_child();   /* never returns */

    int status;
    if (wait_for_child(pid, &status) != 0) return 1;

    if (WIFEXITED(status) && WEXITSTATUS(status) == CHILD_LIMIT_ENFORCED) {
        printf("[parent] Observed effect: child %d exited normally; its 300 MB "
               "allocation was refused by the kernel limit.\n", (int)pid);
        return 0;
    }
    if (WIFEXITED(status))
        printf("[parent] Unexpected: child %d exited with code %d.\n",
               (int)pid, WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("[parent] Unexpected: child %d killed by signal %d (%s).\n",
               (int)pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
    return 1;
}

static void cpu_child(void) {

    struct rlimit core = {0, 0};
    struct rlimit rl = {1, 2};

    setrlimit(RLIMIT_CORE, &core);
    print_limit("before", RLIMIT_CPU);

    // error handling : setrlimit
    if (setrlimit(RLIMIT_CPU, &rl) != 0) {
        fprintf(stderr, "Error: setrlimit(RLIMIT_CPU) failed: %s\n", strerror(errno));
        fflush(stdout);
        _exit(CHILD_SETUP_FAILED);
    }
    print_limit("after ", RLIMIT_CPU);

    printf("  [child] spinning in a busy loop, waiting to be stopped...\n");
    fflush(stdout);
 
    volatile unsigned long counter = 0;
    for (;;) counter++;             /* burns CPU until the kernel intervenes */

}

static int demo_cpu_limit(void)
{
    printf("\n[Demo 2] RLIMIT_CPU: cap the child's CPU time at 1 second\n");
    fflush(stdout);
 
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Error: fork failed: %s\n", strerror(errno));
        return 1;
    }
    if (pid == 0) cpu_child();      /* never returns */
 
    int status;
    if (wait_for_child(pid, &status) != 0) return 1;
 
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGXCPU) {
        printf("[parent] Observed effect: child %d was killed by signal %d (%s) "
               "after using 1 s of CPU time.\n",
               (int)pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
        return 0;
    }
    if (WIFSIGNALED(status))
        printf("[parent] Unexpected: child %d killed by signal %d (%s).\n",
               (int)pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
    else if (WIFEXITED(status))
        printf("[parent] Unexpected: child %d exited with code %d.\n",
               (int)pid, WEXITSTATUS(status));
    return 1;
}
 
/* ---- Entry point ---------------------------------------------------- */
 
int run_limits(int argc, char **argv)
{
    if (argc > 1) {
        fprintf(stderr, "Error: 'limits' takes no arguments (got '%s')\n"
                        "Usage: campuscompute limits\n", argv[1]);
        return 2;
    }
 
    printf("=== Resource limits demo (setrlimit) ===\n");
    int failures = 0;
    failures += demo_memory_limit();
    failures += demo_cpu_limit();
 
    printf("\nCleanup: both child processes were reaped with waitpid(); "
           "no processes, threads or IPC objects remain.\n");
    printf("Result: %s\n", failures == 0 ? "PASS" : "FAIL");
    return failures == 0 ? 0 : 1;
}