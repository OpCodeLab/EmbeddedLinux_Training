#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) 
{
    printf("Thread received signal %d\n", sig);
}

void* thread_func(void* arg)
 {
    // Register signal handler for SIGUSR1
    signal(SIGUSR1, handler);
    printf("Thread: Going to sleep...\n");
    sleep(10); // Sleep, can be interrupted by a signal
    printf("Thread: Woke up\n");
    return NULL;
}

int main()
 {
    pthread_t thread;
    pthread_create(&thread, NULL, thread_func, NULL);

    sleep(1); // Let the thread start
    printf("Main: Sending SIGUSR1 to thread\n");
    pthread_kill(thread, SIGUSR1); // Send SIGUSR1 to the thread

    pthread_join(thread, NULL);
    return 0;
}