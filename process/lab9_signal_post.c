#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>


int main() {
    pid_t pid;
    printf("Enter the target process ID: ");
    scanf("%d", &pid);  // Get the PID of the process to send the signal to

    if (kill(pid, SIGUSR1) == -1) 
	{
        perror("Error sending signal");
    } 
	else 
	{
        printf("Sent SIGUSR1 to process %d\n", pid);
    }

    return 0;
}