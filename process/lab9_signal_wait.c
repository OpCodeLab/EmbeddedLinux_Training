#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void handle_sigusr1(int sig) {
    printf("Received SIGUSR1 signal\n");
    exit(0);  // Exit the program
}

int main() {
    signal(SIGUSR1, handle_sigusr1);

    // Infinite loop to wait for signals
    while (1) {

        printf("Waiting for SIGUSR1...\n");
        pause();  // Wait for any signal to arrive
    }

    return 0;
}
