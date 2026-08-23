/* RACE CONDITION
   This program demonstrates
 */

#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>


#define NUM_THREADS 100
// Atomic counter
  int counter = 0;

// Function to increment the counter
void* increment_counter(void* arg)
 {
    for (int i = 0; i < 1000; ++i)
     {
        atomic_fetch_add(&counter, 1);  // Atomically increment counter
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
    printf("Using Atomic operations for synchronization\n");

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


    
    return 0;
}
