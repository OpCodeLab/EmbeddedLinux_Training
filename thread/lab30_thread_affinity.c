#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

void* thread_func(void* arg) 
{
    printf("Thread running on CPU %d\n", sched_getcpu());
    while (1)
    {
        
    }  // simulate load
    return NULL;
}

int main()
 {
    pthread_t thread;
    cpu_set_t cpuset;

    pthread_create(&thread, NULL, thread_func, NULL);

    // Bind to CPU 2
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset);  // CPU core index

    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("pthread_setaffinity_np");
        exit(1);
    }

    pthread_join(thread, NULL);
    return 0;
}

