#include "stdio.h"
#include <unistd.h>
#include <stdint.h>

#define MAX 100

int main()
{
    uint32_t x=MAX;

    printf("LAB1---------------------- \r\n");
    while(1)
    {
    printf("Hello world from process id =%d my ppid =%d \n",getpid(), getppid());
    }
    return 0;  //ok
    
}