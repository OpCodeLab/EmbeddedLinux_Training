/*LAB6---------------------- */
/*static libary the lib.c will be linked statically */
/*
gcc main.o -L. -l:mylib.a -o mainE
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
