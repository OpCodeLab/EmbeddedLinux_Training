/*LAB7---------------------- */
/*shared library: lib.c is compiled to lib.so and linked dynamically */
/*
gcc main.o -L. -l:lib.so -o mainE -Wl,-rpath,'$ORIGIN/lib'
*/

#include "lib.h"
#include "stdio.h"

uint32_t gtable[20] = {0x154F11D};

int main ()
{
 
   printf("LAB7 Shared lib---------------------- \r\n");
 
  uint32_t crc=compute_crc32(gtable, sizeof(gtable));
  printf("crc32 = %x \n", crc);
  uint32_t max=getMaxValue(gtable, sizeof(gtable)/sizeof(gtable[0]));
  printf("max = %d \n", max);
  
   return 0;
}
