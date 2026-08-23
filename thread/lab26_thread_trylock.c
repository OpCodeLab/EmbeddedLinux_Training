#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

pthread_mutex_t resource_mutex = PTHREAD_MUTEX_INITIALIZER;

void* thread_function(void* arg) 
{
    int thread_id = *((int*)arg);
    int retry_count = 0;
    
    while (retry_count < 5) 
    {
        int result = pthread_mutex_trylock(&resource_mutex);
        
        if (result == 0) 
        {
            // Lock acquired successfully
            printf("Thread %d: Got the lock, working with resource...\n", thread_id);
            sleep(2); // Simulate work with the resource
            pthread_mutex_unlock(&resource_mutex);
            printf("Thread %d: Released the lock\n", thread_id);
            return NULL;
        } 
        else if (result == EBUSY)
         {
            // Lock is busy, do something else instead of waiting
            printf("Thread %d: Resource busy, doing other work instead (attempt %d)\n", 
                   thread_id, retry_count + 1);
            sleep(1); // Do other work
            retry_count++;
        } 
        else 
        {
            // Handle other error cases
            printf("Thread %d: Error trying to lock: %d\n", thread_id, result);
            return NULL;
        }
    }
    
    printf("Thread %d: Giving up after %d attempts\n", thread_id, retry_count);
    return NULL;
}

int main() 
{
    pthread_t threads[3];
    int thread_ids[3] = {1, 2, 3};
    
    // Create threads
    for (int i = 0; i < 3; i++) 
    {
        pthread_create(&threads[i], NULL, thread_function, &thread_ids[i]);
    }
    
    // Join threads
    for (int i = 0; i < 3; i++)
     {
        pthread_join(threads[i], NULL);
    }
    
    pthread_mutex_destroy(&resource_mutex);
    return 0;
}