/* RACE CONDITION
   This program demonstrates
 */

#define _GNU_SOURCE
#define MUTEX
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex;

#define NUM_THREADS 100

// Shared variable that will be incremented by multiple threads
 int counter = 0;
// Function that increments the counter
void* increment_counter(void* arg)
 {
    for (int i = 0; i < 1000; i++) 
    {
      
      // mutual exclusion
         // Lock the mutex before incrementing the counter
         pthread_mutex_lock(&mutex);
        
        // Increment the counter
        counter++;

         //Unlock the mutex after incrementing the counter
         pthread_mutex_unlock(&mutex); 

    }
    return NULL;
}


int main()
 {
    pthread_t threads[NUM_THREADS];
    clock_t start_time, end_time;

    // Initialize the counter
    counter = 0;

    printf("Thread Number: %d\n", NUM_THREADS);
    // Initialize the mutex
    printf("Using Mutex for synchronization\n");

    if (pthread_mutex_init(&mutex, NULL) != 0) 
    {
        printf("Mutex initialization failed\n");
        return -1;
    }

     // Start timing the execution
     start_time = clock();

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, increment_counter, NULL)) {
            printf("Error creating thread %d\n", i);
            return -1;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL)) {
            printf("Error joining thread %d\n", i);
            return -1;
        }
    }

    // Stop timing the execution
    end_time = clock();
    // Calculate the time taken in milliseconds
    // Note: The time is in clock ticks, so we need to convert it to seconds
    double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  
    printf("Time taken to increment counter: %f mseconds\n", time_taken*1000);

    // Print the final value of the counter
    printf("Final counter value: %d\n", counter);

    // Destroy the mutex
    pthread_mutex_destroy(&mutex);
    
    return 0;
}
