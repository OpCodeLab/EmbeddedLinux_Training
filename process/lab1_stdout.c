#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>


int main() 

{
    int fd = open("nonexistent.txt", O_RDONLY);
 
    
    if (fd == -1)
     {
        //perror("open failed");
        //fprintf(stderr, "Open failed\n"); // stderr (fd 2)
         write(2, "open failed\n", 12);  // FD 2 = stderr


    }

    printf("This message will not be printed \n");// stdout (fd 1)
    //write(1, "Hello stdout\n", 13);  // FD 1 = stdout

    return 0;
}


