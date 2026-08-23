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
    sem_wait(sem);  // Lock
    printf("[Writer] Got lock. Writing...\n");

    FILE *f = fopen("/tmp/shared.txt", "a");
    if (f) {
        fprintf(f, "Writer: wrote something.\n");
        fclose(f);
    } else {
        perror("fopen");
    }

    sleep(2);  // Simulate long operation
    printf("[Writer] Done. Releasing lock.\n");

    sem_post(sem);  // Unlock
    sem_close(sem);
    return 0;
}
