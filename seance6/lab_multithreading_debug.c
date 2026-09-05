/*
 * lab_multithreading_debug.c
 *
 * 3 threads:
 *   - periodic_thread : prints a "keepalive" message every PERIODIC_INTERVAL_SEC
 *   - producer_thread  : pushes incrementing items into a bounded queue
 *   - consumer_thread  : pops items from the bounded queue
 *
 * The producer/consumer share a fixed-size circular buffer protected by a
 * mutex, with two condition variables (not_full / not_empty) so the
 * producer blocks when the queue is full and the consumer blocks when it
 * is empty.
 *
 * Press Ctrl+C (SIGINT) to stop all threads cleanly.
 *
 * Build for debugging (VS Code task "build lab_multithreading_debug"):
 *   gcc -g -O0 -pthread -Wall -o out/lab_multithreading_debug lab_multithreading_debug.c
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define QUEUE_CAPACITY          5
#define PERIODIC_INTERVAL_SEC   2
#define PRODUCER_DELAY_MS       400
#define CONSUMER_DELAY_MS       700

/* ============================================================
 * Global state
 * ============================================================ */

static volatile sig_atomic_t g_running = 1;

static pthread_mutex_t queue_mutex   = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond_not_full  = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  cond_not_empty = PTHREAD_COND_INITIALIZER;

static int queue_buffer[QUEUE_CAPACITY];
static int queue_head  = 0;   /* next slot to consume */
static int queue_tail   = 0;  /* next slot to produce */
static int queue_count  = 0;  /* number of items currently stored */

/* ============================================================
 * Helpers
 * ============================================================ */

static void sigint_handler(int signo)
{
    (void)signo;
    g_running = 0;

    /* Wake up any thread blocked in pthread_cond_wait() so it can notice
     * g_running == 0 and exit. */
    pthread_cond_broadcast(&cond_not_full);
    pthread_cond_broadcast(&cond_not_empty);
}

/* ============================================================
 * Periodic thread
 * ============================================================ */

static void *periodic_thread(void *arg)
{
    (void)arg;
    int tick = 0;

    while (g_running)
    {
        printf("[PERIODIC]: keepalive #%d\n", tick++);
        sleep(PERIODIC_INTERVAL_SEC);
    }

    printf("[PERIODIC]: stopping\n");
    return NULL;
}

/* ============================================================
 * Producer thread
 * ============================================================ */

static void *producer_thread(void *arg)
{
    (void)arg;
    int item = 0;

    while (g_running)
    {
        pthread_mutex_lock(&queue_mutex);

        while (queue_count == QUEUE_CAPACITY && g_running)
        {
            printf("[PRODUCER]: queue full, waiting\n");
            pthread_cond_wait(&cond_not_full, &queue_mutex);
        }

        if (!g_running)
        {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        queue_buffer[queue_tail] = item;
        queue_tail = (queue_tail + 1) % QUEUE_CAPACITY;
        queue_count++;

        printf("[PRODUCER]: produced %d (queue_count=%d)\n", item, queue_count);

        pthread_cond_signal(&cond_not_empty);
        pthread_mutex_unlock(&queue_mutex);

        item++;
        usleep(PRODUCER_DELAY_MS * 1000);
    }

    printf("[PRODUCER]: stopping\n");
    return NULL;
}

/* ============================================================
 * Consumer thread
 * ============================================================ */

static void *consumer_thread(void *arg)
{
    (void)arg;

    while (g_running)
    {
        pthread_mutex_lock(&queue_mutex);

        while (queue_count == 0 && g_running)
        {
            printf("[CONSUMER]: queue empty, waiting\n");
            pthread_cond_wait(&cond_not_empty, &queue_mutex);
        }

        if (!g_running)
        {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        int item = queue_buffer[queue_head];
        queue_head = (queue_head + 1) % QUEUE_CAPACITY;
        queue_count--;

        printf("[CONSUMER]: consumed %d (queue_count=%d)\n", item, queue_count);

        pthread_cond_signal(&cond_not_full);
        pthread_mutex_unlock(&queue_mutex);

        usleep(CONSUMER_DELAY_MS * 1000);
    }

    printf("[CONSUMER]: stopping\n");
    return NULL;
}

/* ============================================================
 * main
 * ============================================================ */

int main(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    pthread_t th_periodic, th_producer, th_consumer;

    printf("[MAIN]: starting threads (Ctrl+C to stop)\n");

    pthread_create(&th_periodic, NULL, periodic_thread, NULL);
    pthread_create(&th_producer, NULL, producer_thread, NULL);
    pthread_create(&th_consumer, NULL, consumer_thread, NULL);

    pthread_join(th_periodic, NULL);
    pthread_join(th_producer, NULL);
    pthread_join(th_consumer, NULL);

    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&cond_not_full);
    pthread_cond_destroy(&cond_not_empty);

    printf("[MAIN]: all threads joined, exiting\n");
    return 0;
}
