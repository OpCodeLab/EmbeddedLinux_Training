// mutex_writer.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

#define SEM_NAME "/my_mutex"

int main() 
{
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, 1); // Mutex behavior
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    printf("[Writer] Waiting for lock...\n");


    return 0;
}
