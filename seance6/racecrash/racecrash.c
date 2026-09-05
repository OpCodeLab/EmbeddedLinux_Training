/* racecrash.c -- deliberately racy multithreaded program for a
 * sanitizer/debugging lab.
 *
 * Story: a shared "store" of double samples, grown dynamically with
 * realloc() as producer threads append to it (correctly locked). A
 * separate "monitor" thread computes a rolling average by reading the
 * store -- WITHOUT taking the lock, on the theory that "it's just a
 * read, reads are safe". They are not: realloc() is free to move the
 * backing allocation (and free the old one), so the monitor thread
 * can end up dereferencing memory that a producer thread has just
 * freed out from under it. That's a classic heap-use-after-free
 * hiding behind a data race, and it segfaults -- unpredictably, which
 * is exactly the point: a plain crash tells you *that* something is
 * wrong and nothing about *why*, which is why this lab exists.
 *
 * Build/run modes (see Makefile):
 *   make buggy   -> plain build, crashes (usually within a few seconds)
 *   make asan    -> -fsanitize=address build: turns the use-after-free
 *                    into an immediate, precise report instead of an
 *                    unpredictable segfault
 *   make tsan    -> -fsanitize=thread build: reports the data race
 *                    itself (the unsynchronized read vs. the locked
 *                    write), with both stack traces, even on a run
 *                    that never actually crashes
 *   make fixed   -> compiled with -DUSE_LOCK_ON_READ, i.e. the actual
 *                    fix: the monitor thread takes the same lock
 *                    producers use. Clean under tsan/asan.
 */

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NPRODUCERS         4
#define INITIAL_CAP        4
#define GROWTH_CHUNK       64
#define RUN_SECONDS        10
#define PRODUCER_DELAY_US  200

typedef struct {
    double *data;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} store_t;

static store_t g_store;

/* Note this is atomic_int, not "volatile int": volatile only stops
 * the compiler reordering/caching a variable, it says nothing about
 * memory visibility or ordering between threads and is not a
 * synchronization primitive in C. A plain "volatile int" shutdown
 * flag here is itself a (separate, more benign) data race that
 * ThreadSanitizer will flag even in the "fixed" build if you leave it
 * -- worth demonstrating once, since "I marked it volatile so it's
 * thread-safe" is a very common and very wrong belief. */
static atomic_int g_running = 1;
static struct timespec g_start;

static double seconds_since_start(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)(now.tv_sec - g_start.tv_sec) +
           (double)(now.tv_nsec - g_start.tv_nsec) / 1e9;
}

static void store_init(store_t *s)
{
    s->data = malloc(INITIAL_CAP * sizeof(double));
    s->count = 0;
    s->capacity = INITIAL_CAP;
    pthread_mutex_init(&s->lock, NULL);
}

/* Producer side: correctly locked. Appends one sample, growing the
 * backing array with realloc() when full. */
static void store_push(store_t *s, double value)
{
    pthread_mutex_lock(&s->lock);
    if (s->count == s->capacity) {
        /* Linear growth (not doubling): reallocs happen often and
         * evenly throughout the run, which is what makes the crash
         * below land within a fairly narrow, repeatable time window
         * instead of only at a handful of exponentially-spaced
         * moments. */
        s->capacity += GROWTH_CHUNK;
        double *grown = realloc(s->data, s->capacity * sizeof(double));
        s->data = grown; /* may be a different address; may have freed the old one */
    }
    s->data[s->count++] = value;
    pthread_mutex_unlock(&s->lock);
}

/* Snapshot of both stats the monitor thread cares about, taken
 * together so there's exactly one racy (or, in the fixed build, one
 * locked) access point -- not two, which is its own easy mistake:
 * see the note in monitor_main() below about the printf that used to
 * read g_store.count a second time, separately and unguarded, even in
 * the "fixed" build. */
typedef struct { double avg; size_t count; } stats_t;

/* THE BUG: reads s->data / s->count without holding s->lock. */
static stats_t store_stats_unsafe(store_t *s)
{
    stats_t st;
    st.count = s->count;   /* racy read */
    double *d = s->data;    /* racy read: may already be a freed/stale pointer */
    double sum = 0.0;
    for (size_t i = 0; i < st.count; i++)
        sum += d[i];         /* can dereference freed memory -> segfault, eventually */
    st.avg = st.count ? sum / (double)st.count : 0.0;
    return st;
}

#ifdef USE_LOCK_ON_READ
/* THE FIX: take the same lock producers use before touching the
 * store's fields. Reads need the lock exactly as much as writes do,
 * whenever another thread can mutate the thing you're reading. */
static stats_t store_stats_safe(store_t *s)
{
    pthread_mutex_lock(&s->lock);
    stats_t st = store_stats_unsafe(s);
    pthread_mutex_unlock(&s->lock);
    return st;
}
#endif

static void *producer_main(void *arg)
{
    (void)arg;
    while (atomic_load_explicit(&g_running, memory_order_relaxed)) 
    {
        store_push(&g_store, (double)(rand() % 1000) / 10.0);
        usleep(PRODUCER_DELAY_US);
    }
    return NULL;
}

static void *monitor_main(void *arg)
{
    (void)arg;

    while (atomic_load_explicit(&g_running, memory_order_relaxed)) 
    {
#ifdef USE_LOCK_ON_READ
        stats_t st = store_stats_safe(&g_store);
#else
        stats_t st = store_stats_unsafe(&g_store);
#endif
        /* Use st.count here, NOT g_store.count -- that would be a
         * second unguarded access to the shared struct, alongside the
         * one inside store_stats_*(), and would still race even in
         * the "fixed" build (an earlier draft of this file made
         * exactly that mistake, and ThreadSanitizer still flagged it
         * after the "fix" -- which is itself worth showing engineers:
         * sanitizers catch what code review misses). */
        printf("monitor: t=%.1fs avg=%.2f count=%zu\n",
               seconds_since_start(), st.avg, st.count);
        fflush(stdout); /* so output survives right up to the crash */
        usleep(1000); /* 1ms: frequent enough to collide with growth often */
    }
    return NULL;
}

int main(void)
{
    clock_gettime(CLOCK_MONOTONIC, &g_start);
    store_init(&g_store);
    srand((unsigned)time(NULL));

    pthread_t producers[NPRODUCERS], monitor;


    for (int i = 0; i < NPRODUCERS; i++)
        pthread_create(&producers[i], NULL, producer_main, NULL);


    pthread_create(&monitor, NULL, monitor_main, NULL);

    sleep(RUN_SECONDS);
    atomic_store_explicit(&g_running, 0, memory_order_relaxed);

    for (int i = 0; i < NPRODUCERS; i++)
        pthread_join(producers[i], NULL);


    pthread_join(monitor, NULL);

    printf("clean exit, %zu samples collected\n", g_store.count);
    free(g_store.data);
    pthread_mutex_destroy(&g_store.lock);
    return 0;
}
