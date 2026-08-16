/* Get Pid / PPID WAIT  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[])
{
    int id = fork();

    if (id < 0)
    {
        perror("fork failed");
        exit(1);
    }
       
    if (id == 0) 
    {
        printf("*** Hello from child process! fork() returned id=%d, my PID=%d, my PPID=%d\n", id, getpid(), getppid());
        sleep(3);
        exit(0);
    } 
    else 
    {
        printf("---Hello from parent process! fork() returned child id=%d, my PID=%d\n", id, getpid());

        int status;
        wait( &status); // Wait for the child process to finish

        printf("--- Parent process (PID=%d) is exiting\n", getpid());
    }
    
    return 0;
}
