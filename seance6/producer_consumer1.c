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

static int data_reg;      /* the single shared "hardware register" */

/* ============================================================
 * Helpers
 * ============================================================ */

static void sigint_handler(int signo)
{
    (void)signo;
    g_running = 0;

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


        if (!g_running)
        {
            break;
        }

        //produce 
        data_reg = item;

        printf("[PRODUCER]: wrote %d (TXE=0, RXNE=1)\n", item);

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

        if (!g_running)
        {
            break;
        }

        int item = data_reg;
  

        printf("[CONSUMER]: read %d (RXNE=0, TXE=1)\n", item);



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
    

    pthread_create(&th_producer, NULL, producer_thread, NULL);
    pthread_create(&th_consumer, NULL, consumer_thread, NULL);

    pthread_join(th_producer, NULL);
    pthread_join(th_consumer, NULL);


    printf("[MAIN]: all threads joined, exiting\n");
    return 0;
}