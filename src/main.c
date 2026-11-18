/* FR8 command-line interface (Owner: You) */
#include <stdio.h>
#include <string.h>
#include "adapters.h"
#include "logger.h"
#include "ipc.h"
#include "res_limits.h"
#include "demo.h"

static void usage(void) {
    printf("CampusCompute - usage:\n"
           "  campuscompute help\n"
           "  campuscompute sched fcfs|rr|prio|compare <jobs.txt>\n"
           "  campuscompute logger\n"
           "  campuscompute ipc\n"
           "  campuscompute banker <state.txt>\n"
           "  campuscompute worker ok|fail\n"
           "  campuscompute limits\n"
           "  campuscompute demo\n");
}

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "help") == 0) { usage(); return argc < 2 ? 1 : 0; }
    if (!strcmp(argv[1], "sched"))   return run_sched(argc - 1, argv + 1);
    if (!strcmp(argv[1], "logger"))  return run_logger(argc - 1, argv + 1);
    if (!strcmp(argv[1], "ipc"))     return run_ipc(argc - 1, argv + 1);
    if (!strcmp(argv[1], "banker"))  return run_banker(argc - 1, argv + 1);
    if (!strcmp(argv[1], "worker"))  return run_worker(argc - 1, argv + 1);
    if (!strcmp(argv[1], "limits"))  return run_limits(argc - 1, argv + 1);
    if (!strcmp(argv[1], "demo"))    return run_demo(argc - 1, argv + 1);
    fprintf(stderr, "Error: unknown command '%s'\n\n", argv[1]);
    usage();
    return 2;
}
