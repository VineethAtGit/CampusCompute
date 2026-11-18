#ifndef CC_ADAPTERS_H
#define CC_ADAPTERS_H
/* Command-line entry points for the scheduler, Banker's and worker modules.
 * They parse the input files and call the teammate's functions. (Owner: You) */
int run_sched(int argc, char **argv);
int run_banker(int argc, char **argv);
int run_worker(int argc, char **argv);
#endif
