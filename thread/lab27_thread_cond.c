
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Shared resources
pthread_mutex_t mutex;
pthread_cond_t cond;
int shared_value = 0; // Shared variable

// Thread that waits for a specific value
void* waiter(void* arg)
 {
    int desired_value = 42; // The value to wait for
    pthread_mutex_lock(&mutex);

    // Wait until shared_value reaches the desired value
    while (shared_value != desired_value) {
        printf("Waiter: shared_value = %d, waiting for %d...\n", shared_value, desired_value);
        pthread_cond_wait(&cond, &mutex);
    }

    // Condition is met (shared_value == desired_value)
    printf("============ > Waiter: Condition met! shared_value = %d\n", shared_value);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

// Thread that updates the shared value and signals
void* updater(void* arg)
 {
    sleep(2); // Simulate some work
    shared_value = 39;

     for (int i=0; i<5; i++)
     {
        printf("Updater: Updating shared_value...\n");
        sleep(1); // Simulate some work
        pthread_mutex_lock(&mutex);
        // Update the shared variable to the desired value
        printf("Updater: Set shared_value to %d\n", shared_value);
        shared_value++;
        // Signal the waiting thread
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
     }

    return NULL;
}

int main() 
{
    pthread_t waiter_thread, updater_thread;

    // Initialize mutex and condition variable
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    // Create the waiter and updater threads
    pthread_create(&waiter_thread, NULL, waiter, NULL);
    pthread_create(&updater_thread, NULL, updater, NULL);

    // Wait for both threads to finish
    pthread_join(waiter_thread, NULL);
    pthread_join(updater_thread, NULL);

    // Clean up
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}