/* LAB18 : Thread Creation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>
   

void *myTheadFunction(void * args)
{

  char * str = (char *) args;

  printf("%s", str);

  while(1) 
   {
     sleep(1);
     printf("Thread is still running...\n");
    }

  return NULL;
}

int main(int argc, char * argv[])
{

  char hello[] = "Hello World!\n";

  pthread_t thread;  //thread identifier

  //create a new thread that runs hello_arg with argument hello
  pthread_create(&thread, NULL, myTheadFunction, hello);

  //wait until the thread completes
  pthread_join(thread, NULL);

  return 0;
}