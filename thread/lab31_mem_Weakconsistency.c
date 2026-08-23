#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int x = 0, y = 0;
int r1 = 0, r2 = 0;

void *thread1(void *arg)
{
    x = 1;   
    r1 = y;
    return NULL;
}

void *thread2(void *arg)
{
    y = 1;
    r2 = x;
    return NULL;
}

int main()
{
    int i;
    int race_detected = 0;

    for (i = 0; i < 100000; i++) 
    {
        pthread_t t1, t2;

        // Reset variables
        x = 0; y = 0;
        r1 = 0; r2 = 0;

        // Create threads
        pthread_create(&t1, NULL, thread1, NULL);
        pthread_create(&t2, NULL, thread2, NULL);

        // Wait for threads to finish
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);

        // Check the result
        if (r1 == 0 && r2 == 0) 
        {
            printf("Race detected at iteration %d: r1 = %d, r2 = %d\n", i, r1, r2);
            race_detected++;
        }
    }

    printf("\nTotal races detected: %d out of 100000\n", race_detected);

    return 0;
}
