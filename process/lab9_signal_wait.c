#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void handle_sig(int sig) 
{
    
    printf("Received signal %d\n", sig);
   switch (sig) {
        case SIGUSR1:
            printf("Handling SIGUSR1...\n");
                exit(0);  // Exit the program

            break;
        case SIGINT:
            printf("Ignoring SIGINT (Ctrl+C)...\n");
            break;
        default:
            printf("Unhandled signal: %d\n", sig);
    }

}

int main()
 {
    
    signal(SIGUSR1, handle_sig);
   signal(SIGINT, handle_sig);  // Ignore SIGINT (Ctrl+C)

    printf ("Process ID: %d\n", getpid());
    // Infinite loop to wait for signals
    while (1) {

        printf("Waiting for SIGUSR1...\n");
        pause();  // Wait for any signal to arrive
    }

    return 0;
}
