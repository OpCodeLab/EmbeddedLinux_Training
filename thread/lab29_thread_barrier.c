#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 4

pthread_barrier_t barrier;

void* thread_func(void* arg)
 {


    printf("Thread %lu: Waiting at the barrier...\n",pthread_self());
    pthread_barrier_wait(&barrier);


    // Do more work after the barrier
    printf("Thread %lu: Doing work after the barrier.\n", pthread_self());
    free(arg);
    return NULL;
}

int main(int argc, char* argv[])
  {
    pthread_t threads[NUM_THREADS];

    // Initialize the barrier for NUM_THREADS threads
    if (pthread_barrier_init(&barrier, NULL, NUM_THREADS) != 0)
     {
        perror("pthread_barrier_init");
        return EXIT_FAILURE;
    }

    /*create NUM_THREAD -1 thread */
    for (int i = 0; i < NUM_THREADS-1; ++i)
     {

        if (pthread_create(&threads[i], NULL, thread_func, NULL) != 0) 
        {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_THREADS; ++i) 
    {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    return EXIT_SUCCESS;
  }