/*LAB4---------------------- */
/*static libary the lib.c will be linked statically */
/*
gcc -c main.c -o main.o
gcc -c lib.c -o lib.o
gcc main.o lib.o -o main
*/
#include "lib.h"
#include "stdio.h"

uint32_t gtable[20] = {0x154F11D};

int main ()
{
 
   printf("LAB4---------------------- \r\n");
 
  uint32_t crc=compute_crc32(gtable, sizeof(gtable));
  printf("crc32 = %x \n", crc);
  uint32_t max=getMaxValue(gtable, sizeof(gtable)/sizeof(gtable[0]));
  printf("max = %d \n", max);
  
   return 0;
}
