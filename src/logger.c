#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>

#include "logger.h"

#define QUEUE_CAP           4
#define NUM_PRODUCERS       3
#define MSGS_PER_PRODUCER   5
#define LOG_PATH            "logs/campus.log"

typedef struct {
    int     producer;
    int     seq;
    int     poison;
    char    text[64];
} LogEntry;

typedef struct {
    LogEntry        buf[QUEUE_CAP];
    int             head, tail, count;
    pthread_mutex_t lock;
    pthread_cond_t  not_full;
    pthread_cond_t  not_empty;
    FILE            *log;
    int             produced;
    int             consumed;
} Queue;

typedef struct {
    Queue *q;
    int   id;
} ProducerArg;

static void sleep_ms(long ms) {
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

static void queue_push(Queue *q, const LogEntry *e) {
    pthread_mutex_lock(&q->lock);
    if(q->count == QUEUE_CAP && !e->poison) {
        printf("    [producer %d] queue FULL (%d/%d), waiting...n", 
                e->producer, q->count, QUEUE_CAP);
    }
    while(q->count == QUEUE_CAP)
            pthread_cond_wait(&q->not_full, &q->lock);

    q->buf[q->tail] = *e;
    q->tail = (q->tail + 1) % QUEUE_CAP;
    q->count++;
    if (!e->poison) {
        q->produced++;
        printf("  [producer %d] queued event #%d  (queue %d/%d)\n",
               e->producer, e->seq, q->count, QUEUE_CAP);
    }
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

static LogEntry queue_pop(Queue *q) {
    pthread_mutex_lock(&q->lock);
    while(q->count == 0)
        pthread_cond_wait(&q->not_empty, &q->lock);
    
    LogEntry e = q->buf[q->head];
    q->head = (q->head + 1) % QUEUE_CAP;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return e;
}

static void *producer_thread(void *arg) {
    ProducerArg *pa = arg;
    for(int i = 1; i <= MSGS_PER_PRODUCER; i++) {
        LogEntry e;
        memset(&e, 0, sizeof e);
        e.producer = pa->id;
        e.seq = i;
        snprintf(e.text, sizeof e.text, "worker %d reported step %d", pa->id, i);
        queue_push(pa->q, &e);

    }
}

static void *consumer_thread(void *arg)
{
    Queue *q = arg;
    for (;;) {
        LogEntry e = queue_pop(q);
        if (e.poison) break;              /* shutdown marker: stop cleanly */
 
        char stamp[32];
        time_t now = time(NULL);
        struct tm tmv;
        localtime_r(&now, &tmv);
        strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", &tmv);
 
        fprintf(q->log, "%s [P%d #%d] %s\n", stamp, e.producer, e.seq, e.text);
        fflush(q->log);
        q->consumed++;
        printf("  [consumer]   wrote  P%d #%d to %s\n", e.producer, e.seq, LOG_PATH);
        sleep_ms(40);                     /* slow consumer -> queue fills up */
    }
    return NULL;
}

int run_logger(int argc, char **argv)
{
    if (argc > 1) {
        fprintf(stderr, "Error: 'logger' takes no arguments (got '%s')\n"
                        "Usage: campuscompute logger\n", argv[1]);
        return 2;
    }
 
    if (mkdir("logs", 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error: cannot create logs/ directory: %s\n", strerror(errno));
        return 1;
    }
 
    Queue q;
    memset(&q, 0, sizeof q);
    q.log = fopen(LOG_PATH, "w");
    if (!q.log) {
        fprintf(stderr, "Error: cannot open %s: %s\n", LOG_PATH, strerror(errno));
        return 1;
    }
    pthread_mutex_init(&q.lock, NULL);
    pthread_cond_init(&q.not_full, NULL);
    pthread_cond_init(&q.not_empty, NULL);
 
    printf("=== Concurrent logger: %d producers, queue capacity %d, 1 consumer ===\n",
           NUM_PRODUCERS, QUEUE_CAP);
    fprintf(q.log, "# CampusCompute event log\n");
 
    int rc = 0;
    pthread_t consumer, producers[NUM_PRODUCERS];
    ProducerArg args[NUM_PRODUCERS];
    int started = 0;
 
    if (pthread_create(&consumer, NULL, consumer_thread, &q) != 0) {
        fprintf(stderr, "Error: cannot create consumer thread\n");
        rc = 1;
        goto cleanup;
    }
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        args[i].q = &q;
        args[i].id = i + 1;
        if (pthread_create(&producers[i], NULL, producer_thread, &args[i]) != 0) {
            fprintf(stderr, "Error: cannot create producer thread %d\n", i + 1);
            rc = 1;
            break;
        }
        started++;
    }
 
    for (int i = 0; i < started; i++)         /* wait for every producer */
        pthread_join(producers[i], NULL);
 
    LogEntry pill;
    memset(&pill, 0, sizeof pill);
    pill.poison = 1;
    queue_push(&q, &pill);                    /* tell consumer to finish */
    pthread_join(consumer, NULL);
 
    printf("\nAll %d producers and the consumer have been joined.\n", started);
    printf("Events produced: %d, events written to log: %d\n", q.produced, q.consumed);
    fprintf(q.log, "# end of log: %d entries\n", q.consumed);
 
    if (rc == 0 && (q.produced != q.consumed ||
                    q.produced != NUM_PRODUCERS * MSGS_PER_PRODUCER)) {
        fprintf(stderr, "Error: produced and consumed counts do not match\n");
        rc = 1;
    }
    printf("Result: %s\n", rc == 0 ? "PASS" : "FAIL");
 
cleanup:
    pthread_cond_destroy(&q.not_full);
    pthread_cond_destroy(&q.not_empty);
    pthread_mutex_destroy(&q.lock);
    fclose(q.log);
    return rc;
}