#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#if 0
int main()
 {
    int status = system("ls -l");


    FILE *fp;
    char path[128];

    fp = popen("uname -a", "r");
    if (fp == NULL) {
        perror("popen failed");
        return 1;
    }

    while (fgets(path, sizeof(path), fp) != NULL) {
        printf("Output: %s", path);
    }

    pclose(fp);


    return 0;

    
}


#else

int main() {
    pid_t pid = fork();

    if (pid == 0) 
    {
        // In child
        execlp("ls", "ls", "-l", NULL);
        perror("execlp failed"); // If exec fails
        exit(1);
    } 
    else if (pid > 0)
     {
        // In parent
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status))
         {
            printf("Child exited with status %d\n", WEXITSTATUS(status));
        }
    }
     else
      {
        perror("fork failed");
    }

    return 0;
}
#endif 