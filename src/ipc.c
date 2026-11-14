/* FR4 inter-process communication  (Owner: You)
 *
 * A parent (producer) and a child (consumer) exchange numbers through
 * POSIX shared memory. Two named semaphores coordinate the hand-off of the
 * single-slot mailbox (classic empty/full pattern):
 *   /campus_sem_empty  starts at 1 : the slot may be written
 *   /campus_sem_full   starts at 0 : the slot holds data to read
 * The child attaches to the shared memory and semaphores BY NAME, adds up
 * the numbers it receives and stores the sum back in shared memory.
 * Every IPC object is closed and unlinked afterwards, and that is verified.
 */

 #include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
 
#include "ipc.h"

#define SHM_NAME   "/campus_shm"
#define SEM_EMPTY  "/campus_sem_empty"
#define SEM_FULL   "/campus_sem_full"
#define NUM_ITEMS. "/campus_sem_full"
#define NUM_ITEMS  5

typedef struct {
    int  value;
    int  last;
    long result;
} Mailbox;

static const int ITEMS[NUM_ITEMS] = {10, 20, 30, 40, 50};

static void unlink_all(void) {

    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    shm_unlink(SHM_NAME);

}

static void child_main(void) {
    
    int fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd < 0) { perror("  [child] shm_open"); _exit(1); }
    Mailbox *mb = mmap(NULL, sizeof *mb, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mb == MAP_FAILED) { perror("  [child] mmap"); _exit(1); }
 
    sem_t *empty = sem_open(SEM_EMPTY, 0);
    sem_t *full  = sem_open(SEM_FULL, 0);
    if (empty == SEM_FAILED || full == SEM_FAILED) {
        perror("  [child] sem_open");
        _exit(1);
    }
    printf("  [child %d] attached to shared memory and semaphores by name\n", (int)getpid());
    fflush(stdout);
 
    long sum = 0;
    for (;;) {
        sem_wait(full);                      /* wait until data is available */
        int v = mb->value;
        int last = mb->last;
        sum += v;
        printf("  [child]  received %d  (running sum %ld)\n", v, sum);
        fflush(stdout);
        sem_post(empty);                     /* slot is free again */
        if (last) break;
    }
    mb->result = sum;                        /* answer goes back via shared memory */
 
    sem_close(empty);
    sem_close(full);
    munmap(mb, sizeof *mb);
    _exit(0);
}
 
/* After unlinking, the names must no longer exist. Returns 0 if all gone. */
static int verify_cleanup(void)
{
    int leaks = 0;
 
    int fd = shm_open(SHM_NAME, O_RDONLY, 0);
    if (fd >= 0) { close(fd); leaks++; printf("  LEAK: %s still exists\n", SHM_NAME); }
    else printf("  verified: %s removed\n", SHM_NAME);
 
    sem_t *s = sem_open(SEM_EMPTY, 0);
    if (s != SEM_FAILED) { sem_close(s); leaks++; printf("  LEAK: %s still exists\n", SEM_EMPTY); }
    else printf("  verified: %s removed\n", SEM_EMPTY);
 
    s = sem_open(SEM_FULL, 0);
    if (s != SEM_FAILED) { sem_close(s); leaks++; printf("  LEAK: %s still exists\n", SEM_FULL); }
    else printf("  verified: %s removed\n", SEM_FULL);
 
    return leaks;
}
 
int run_ipc(int argc, char **argv)
{
    if (argc > 1) {
        fprintf(stderr, "Error: 'ipc' takes no arguments (got '%s')\n"
                        "Usage: campuscompute ipc\n", argv[1]);
        return 2;
    }
 
    printf("=== IPC demo: shared memory + named semaphores ===\n");
    int rc = 1;
    int shm_created = 0;
    Mailbox *mb = MAP_FAILED;
    sem_t *empty = SEM_FAILED, *full = SEM_FAILED;
    pid_t pid = -1;
 
    unlink_all();
 
    int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        fprintf(stderr, "Error: shm_open failed: %s\n", strerror(errno));
        goto cleanup;
    }
    shm_created = 1;
    if (ftruncate(fd, sizeof(Mailbox)) != 0) {
        fprintf(stderr, "Error: ftruncate failed: %s\n", strerror(errno));
        close(fd);
        goto cleanup;
    }
    mb = mmap(NULL, sizeof *mb, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mb == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed: %s\n", strerror(errno));
        goto cleanup;
    }
    memset(mb, 0, sizeof *mb);
 
    empty = sem_open(SEM_EMPTY, O_CREAT | O_EXCL, 0600, 1);
    full  = sem_open(SEM_FULL,  O_CREAT | O_EXCL, 0600, 0);
    if (empty == SEM_FAILED || full == SEM_FAILED) {
        fprintf(stderr, "Error: sem_open failed: %s\n", strerror(errno));
        goto cleanup;
    }
    printf("  [parent] created %s, %s, %s\n", SHM_NAME, SEM_EMPTY, SEM_FULL);
    fflush(stdout);
 
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Error: fork failed: %s\n", strerror(errno));
        goto cleanup;
    }
    if (pid == 0) child_main();              /* never returns */
 
    long expected = 0;
    for (int i = 0; i < NUM_ITEMS; i++) {
        sem_wait(empty);                     /* wait until the slot is free */
        mb->value = ITEMS[i];
        mb->last = (i == NUM_ITEMS - 1);
        expected += ITEMS[i];
        printf("  [parent] sent     %d\n", ITEMS[i]);
        fflush(stdout);
        sem_post(full);                      /* tell the child data is ready */
    }
 
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) { fprintf(stderr, "Error: waitpid failed\n"); goto cleanup; }
    }
    pid = -1;                                /* reaped */
 
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0 && mb->result == expected) {
        printf("  [parent] child finished; sum from shared memory = %ld (expected %ld)\n",
               mb->result, expected);
        rc = 0;
    } else {
        fprintf(stderr, "Error: child failed or returned a wrong sum\n");
    }
 
cleanup:
    if (pid > 0) {                           /* child still running after an error */
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }
    if (empty != SEM_FAILED) sem_close(empty);
    if (full  != SEM_FAILED) sem_close(full);
    if (mb != MAP_FAILED) munmap(mb, sizeof *mb);
    if (shm_created || empty != SEM_FAILED || full != SEM_FAILED)
        unlink_all();
 
    printf("Cleanup of IPC resources:\n");
    if (verify_cleanup() != 0) rc = 1;
    printf("Result: %s\n", rc == 0 ? "PASS" : "FAIL");
    return rc;
}