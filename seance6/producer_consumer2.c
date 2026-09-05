/*
 *   - producer_thread  : "transmitter" - writes one word into a single
 *                         shared register, then waits for it to be read
 *   - consumer_thread  : "receiver"    - reads that one word, then waits
 *                         for a new one to be written
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define PRODUCER_DELAY_MS       400
#define CONSUMER_DELAY_MS       700

/* ============================================================
 * Global state
 * ============================================================ */

static volatile sig_atomic_t g_running = 1;

static pthread_mutex_t lock      = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond_txe  = PTHREAD_COND_INITIALIZER; /* signaled when TXE  becomes 1 */
static pthread_cond_t  cond_rxne = PTHREAD_COND_INITIALIZER; /* signaled when RXNE becomes 1 */

static int data_reg;      /* the single shared "hardware register" */
static int TXE  = 1;      /* 1 = data_reg empty,        producer may write */
static int RXNE = 0;      /* 1 = data_reg holds data,   consumer may read  */

/* ============================================================
 * Helpers
 * ============================================================ */

static void sigint_handler(int signo)
{
    (void)signo;
    g_running = 0;

    /* Wake up any thread blocked in pthread_cond_wait() so it can notice
     * g_running == 0 and exit. */
    pthread_cond_broadcast(&cond_txe);
    pthread_cond_broadcast(&cond_rxne);
}



/* ============================================================
 * Producer thread ("transmitter")
 *   1) wait for TXE == 1  (register empty)
 *   2) write data_reg
 *   3) TXE = 0, RXNE = 1, wake the consumer
 * ============================================================ */

static void *producer_thread(void *arg)
{
    (void)arg;
    int item = 0;

    while (g_running)
    {
        pthread_mutex_lock(&lock);

        while (!TXE && g_running)
        {
            printf("[PRODUCER]: TXE=0, waiting for register to be read\n");
            pthread_cond_wait(&cond_txe, &lock);
        }

        if (!g_running)
        {
            pthread_mutex_unlock(&lock);
            break;
        }

        data_reg = item;
        TXE  = 0;
        RXNE = 1;

        printf("[PRODUCER]: wrote %d (TXE=0, RXNE=1)\n", item);

        pthread_cond_signal(&cond_rxne);
        pthread_mutex_unlock(&lock);

        item++;
        usleep(PRODUCER_DELAY_MS * 1000);
    }

    printf("[PRODUCER]: stopping\n");
    return NULL;
}

/* ============================================================
 * Consumer thread ("receiver")
 *   1) wait for RXNE == 1  (register holds unread data)
 *   2) read data_reg
 *   3) RXNE = 0, TXE = 1, wake the producer
 * ============================================================ */

static void *consumer_thread(void *arg)
{
    (void)arg;

    while (g_running)
    {
        pthread_mutex_lock(&lock);

        while (!RXNE && g_running)
        {
            printf("[CONSUMER]: RXNE=0, waiting for data\n");
            pthread_cond_wait(&cond_rxne, &lock);
        }

        if (!g_running)
        {
            pthread_mutex_unlock(&lock);
            break;
        }

        int item = data_reg;
        RXNE = 0;
        TXE  = 1;

        printf("[CONSUMER]: read %d (RXNE=0, TXE=1)\n", item);

        pthread_cond_signal(&cond_txe);
        pthread_mutex_unlock(&lock);

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

    pthread_t th_producer, th_consumer;

    printf("[MAIN]: starting threads (Ctrl+C to stop)\n");
    printf("[MAIN]: initial state TXE=%d RXNE=%d\n", TXE, RXNE);

    pthread_create(&th_producer, NULL, producer_thread, NULL);
    pthread_create(&th_consumer, NULL, consumer_thread, NULL);

    pthread_join(th_producer, NULL);
    pthread_join(th_consumer, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond_txe);
    pthread_cond_destroy(&cond_rxne);

    printf("[MAIN]: all threads joined, exiting\n");
    return 0;
}