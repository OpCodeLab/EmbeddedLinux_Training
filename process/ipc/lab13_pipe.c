#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int x=152;

int main(int argc, char* argv[]) {
    
   // create pipe       
    int fd[2];
    if (pipe(fd) == -1) {
        perror("Pipe failed");
        exit(1);
    }
    
    int id = fork();
    int n;
     
   // Check if fork() failed
    if (id < 0) {
        perror("Fork failed");
        exit(1);
    }
   
    if (id == 0)
     { 
        // Child process
   
    }
    else
     { // Parent process
   
          x++;
   
    }

    return 0;
}