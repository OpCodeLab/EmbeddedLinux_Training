#include "stdio.h"
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

/*global variable    */
uint32_t gtable[256] = {0};
const uint32_t gtable_size = sizeof(gtable)/sizeof(gtable[0]);

// Run time ./lab3
/*real    0m1.005s  == with sleep 1s
user    0m0.004s  
sys     0m0.002s*/
int main()
{
    uint32_t local_table[256] = {0};
    uint32_t *ptable=malloc(sizeof(uint32_t)*gtable_size);


    printf("LAB3---------------------- \r\n");
    printf("Hello world from process id =%d my ppid =%d \n",getpid(), getppid());
     
    for (uint32_t i = 0; i < gtable_size; i++)
    {
        gtable[i] = i;
        local_table[i] =gtable[i] +1;
        ptable[i] = gtable[i] +2;
    }

    for (uint32_t i = 0; i < gtable_size; i++)
    {
        printf("local_table[%d] = %d \n", i, local_table[i]);
        printf("ptable[%d] = %d \n", i, ptable[i]);
    }
    sleep(1);
    free(ptable);
    return 0;
}