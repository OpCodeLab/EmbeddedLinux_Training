#include "stdio.h"
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

/*global variable    */
uint32_t gtable[256] = {0};
const uint32_t gtable_size = sizeof(gtable)/sizeof(gtable[0]);
uint32_t x=15;  ///=== > .DAta

int main()
{
    uint32_t local_table[256] = {0};
    uint32_t *ptable=malloc(sizeof(uint32_t)*gtable_size);


    printf("LAB2---------------------- \r\n");
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
    free(ptable);
    return 0;
}