#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define NUM_THREADS 4 // Number of threads
#define ITERATIONS 100000000 // Number of increments per thread

// Structure to demonstrate false sharing
// Without padding, multiple 'value' members will likely be in the same cache line
typedef struct {
    long long value;
} counter_no_padding_t;

volatile counter_no_padding_t counters[NUM_THREADS];

// Thread function
void *thread_func(void *arg)
 {
    long long thread_id = (long long)arg;

    for (long long i = 0; i < ITERATIONS; ++i) 
    {
        counters[thread_id].value++;
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    struct timeval start, end;
    double elapsed_time;

    printf("Running with %d threads and %d iterations per thread...\n", NUM_THREADS, ITERATIONS);

    // Initialize counters to 0
    for (int i = 0; i < NUM_THREADS; ++i) {
        counters[i].value = 0;
    }

    gettimeofday(&start, NULL);

    // Create threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_create(&threads[i], NULL, thread_func, (void *)(long long)i) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    // Join threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }
    }

    gettimeofday(&end, NULL);

    // Calculate elapsed time
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("All threads finished.\n");
    printf("Total time taken: %.4f seconds\n", elapsed_time);

    // Verify results
    long long total_sum = 0;
    for (int i = 0; i < NUM_THREADS; ++i) {
        printf("Counter %d: %lld\n", i, counters[i].value);
        total_sum += counters[i].value;
    }
    printf("Expected total sum: %lld\n", (long long)NUM_THREADS * ITERATIONS);
    printf("Actual total sum: %lld\n", total_sum);

    if (total_sum == (long long)NUM_THREADS * ITERATIONS) 
    {
        printf("Result is correct.\n");
    } 
    else 
    {
        printf("Result is INCORRECT!\n");
    }

    return 0;
}