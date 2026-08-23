#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(void) {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* CHILD: writer. Redirect its stdout into the pipe's write end */
        close(fd[0]);            /* not reading, close read end */

        if (dup2(fd[1], STDOUT_FILENO) == -1)
         {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(fd[1]);             /* original fd no longer needed once duped */

        /* Anything printed to stdout now goes through the pipe */
        printf("Hello from child via stdout\n");
        fflush(stdout);

        exit(EXIT_SUCCESS);
    } else {
        /* PARENT: reader */
        close(fd[1]);             /* not writing, close write end */

        if (dup2(fd[0], STDIN_FILENO) == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }


        char buf[256];
        ssize_t n;
        while ((n = scanf("%255s", buf)) != EOF) {
            buf[n] = '\0';
            printf("Parent received: %s", buf);
        }
        close(fd[0]);

        wait(NULL);                /* reap child */
    }

    return 0;
}