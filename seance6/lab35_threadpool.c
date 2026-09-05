#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define QUEUE_SIZE       256
#define THREAD_POOL_SIZE 1
#define NUM_TASKS        300


typedef struct
{
    void (*function)(void *);
} TaskTypeDef;


/* ============================ ================================
 * Global thread-pool state
 * ============================================================ */

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

TaskTypeDef taskQueue[QUEUE_SIZE];

int taskCount = 0;

/*
 * shutdown == 0:
 *     Thread pool is running.
 *
 * shutdown == 1:
 *     No more tasks will be submitted.
 *     Workers must finish remaining tasks and exit.
 */
int shutdown = 0;


/* ============================================================
 * User task
 * ============================================================ */

void userFunction(void *arg)
{
    (void)arg;

    int a = rand() % 100;
    int b = rand() % 100;

    int result = a + b;
    int prod = a * b;

    printf("Task: %d + %d = %d, Prod = %d\n",
           a, b, result, prod);

    usleep(50000);
}


/* ============================================================
 * Execute one task
 * ============================================================ */

void executeTask(TaskTypeDef *task)
{
    if (task->function != NULL)
    {
        task->function(NULL);
    }
}


/* ============================================================
 * Submit a task
 * ============================================================ */

void submitTask(TaskTypeDef *task)
{
    pthread_mutex_lock(&mutex);

    if (taskCount < QUEUE_SIZE)
    {
        taskQueue[taskCount] = *task;
        taskCount++;

        /*
         * Wake one worker waiting for a task.
         */
        pthread_cond_signal(&cond);
    }
    else
    {
        printf("Task queue is full!\n");
    }

    pthread_mutex_unlock(&mutex);
}


/* ============================================================
 * Worker thread
 * ============================================================ */

void *startThread(void *arg)
{
    (void)arg;




    while (1)
    {
        TaskTypeDef task;

             pthread_mutex_lock(&mutex);

         
    


        /*
         * Wait while:
         *
         *   queue is empty
         *   AND
         *   shutdown has not been requested
         *
         * The worker sleeps here instead of consuming CPU.
         */
        while (taskCount == 0 && !shutdown)
        {
            pthread_cond_wait(&cond, &mutex);
        }


        /*
         * If shutdown was requested AND there are no
         * remaining tasks, this worker can terminate.
         */
        if (taskCount == 0 && shutdown)
        {
            pthread_mutex_unlock(&mutex);

            printf("Worker exiting...\n");

            break;
        }


        /*
         * Get the first task from the queue.
         */
        task = taskQueue[0];


        /*
         * Remove task from queue.
         */
        for (int i = 0; i < taskCount - 1; i++)
        {
            taskQueue[i] = taskQueue[i + 1];
        }

        taskCount--;

        pthread_mutex_unlock(&mutex);


        /*
         * Execute the task WITHOUT holding the mutex.
         *
         * This is important: the task might take a long time.
         */
        executeTask(&task);
    }

    return NULL;
}


/* ============================================================
 * Shutdown thread pool
 * ============================================================ */

void shutdownThreadPool(void)
{
    pthread_mutex_lock(&mutex);

    /*
     * Tell workers that no more tasks will arrive.
     */
    shutdown = 1;

    /*
     * Wake ALL workers.
     *
     * Some workers may currently be sleeping in
     * pthread_cond_wait().
     */
    pthread_cond_broadcast(&cond);

    pthread_mutex_unlock(&mutex);
}


/* ============================================================
 * Main
 * ============================================================ */

int main(void)
{
    pthread_t threads[THREAD_POOL_SIZE];

    printf("Creating thread pool...\n");


    /* --------------------------------------------------------
     * Create worker threads
     * -------------------------------------------------------- */

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        int ret = pthread_create(
            &threads[i],
            NULL,
            startThread,
            NULL
        );

        if (ret != 0)
        {
            fprintf(stderr,
                    "pthread_create failed\n");

            return 1;
        }
    }

    printf("Thread pool created with %d workers.\n",
           THREAD_POOL_SIZE);


    /* --------------------------------------------------------
     * Submit tasks
     * -------------------------------------------------------- */

    for (int i = 0; i < NUM_TASKS; i++)
    {
        TaskTypeDef task;

        task.function = userFunction;

        submitTask(&task);
    }

    printf("All %d tasks submitted.\n", NUM_TASKS);


    /* --------------------------------------------------------
     * Request shutdown
     * -------------------------------------------------------- */

    printf("Requesting thread-pool shutdown...\n");

    shutdownThreadPool();


    /* --------------------------------------------------------
     * Wait for workers
     * -------------------------------------------------------- */

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("All workers terminated.\n");

    while(1)
    {
        printf("Press Ctrl+C to exit...\n");
        sleep(1);
    }
    /* --------------------------------------------------------
     * Cleanup
     * -------------------------------------------------------- */

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    printf("Thread pool terminated successfully.\n");

    return 0;
}