#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>





void handle_sigsegv(int sig, siginfo_t *si, void *unused)
 {
    printf("Fault address: %p\n", si->si_addr);

    _exit(1);  // Use _exit to avoid calling unsafe functions
}

int main() 
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handle_sigsegv;
    sa.sa_flags = SA_SIGINFO;
    
    // Set up the signal handler for SIGSEGV  POSIX API
    sigaction(SIGSEGV, &sa, NULL);

    int *ptr = (int*)0x400; //invalid address
    *ptr = 123;  // Will cause SIGSEGV

    return 0;
}
