/*exit_IO_demo.c*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void out(void)
{
  printf ("atexit succeeded ! \n");
}

int main()
{

  /*registe callback */
  atexit(out);

  // I/O buffered since no newline
  printf("Will print \n");
   
    // I/O buffered since no newline
    printf("Will not     print");
   

    exit(0);
 //   _exit(0);

 //return 0;

}
