/* LAB19 : Thread Detached */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>


void * hello_arg(void * args)
{

  char * str = (char *) args;
  sleep(1); // Sleep for 1 second to simulate work
  printf("%s", str);

  return NULL;
}

int main(int argc, char * argv[]){

  char hello[] = "Hello World!\n";

  pthread_t thread;  //thread identifier

  //detached thread
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  
    //create a new thread that runs hello_arg with argument hello 
    pthread_create(&thread, &attr, hello_arg, hello);   
 
    pthread_attr_destroy(&attr); // Destroy the attribute object
   

    pthread_exit(0); // Let main thread exit but keep the process alive
    // Process will wait for detached_worker to finish before exiting
  
  return 0;
}