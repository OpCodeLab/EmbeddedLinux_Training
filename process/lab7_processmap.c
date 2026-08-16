#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>
#include <errno.h>

int main() 
{

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) 
    {
        // Child process
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(1, &mask); // Assign to CPU 1

        if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) 
        {
            perror("sched_setaffinity (child)");
            exit(EXIT_FAILURE);
        }

        printf("Child process: Running on CPU %d\n", sched_getcpu());

        // Simulate child work
        while (1) 
        {
            sleep(1); // Sleep to simulate work
        }

    } 
    else 
    {
        // Parent process
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(0, &mask); // Assign to CPU 0

        if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) 
        {
            perror("sched_setaffinity (parent)");
            exit(EXIT_FAILURE);
        }

        printf("Parent process: Running on CPU %d\n", sched_getcpu());

        // Wait for child to finish (infinite loop in this example)
        wait(NULL);
    }

    return 0;
}