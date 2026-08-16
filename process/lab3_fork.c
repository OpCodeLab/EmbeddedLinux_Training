/* Get Pid / PPID WAIT  */
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    int id = fork();
       
    printf("Hello all \r\n");

    if (id == 0) 
    {
        printf("*** Hello from child process! fork() returned id=%d, my PID=%d, my PPID=%d\n", id, getpid(), getppid());
        sleep(3);
        return 1;
    } 
    else 
    {
        printf("---Hello from parent process! fork() returned child id=%d, my PID=%d\n", id, getpid());

        int status;
        wait( &status); // Wait for the child process to finish

        printf("Status of child process: %d\n", status);
        printf("--- Parent process (PID=%d) is exiting\n", getpid());
    }
    
    return 0;
}
