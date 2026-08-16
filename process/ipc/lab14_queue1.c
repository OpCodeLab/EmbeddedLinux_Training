#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <x86intrin.h>
#include <sched.h>

#define QUEUE_NAME  "/myqueue2"
#define MAX_SIZE    1024
unsigned long long start, end;
int main()
 {
    mqd_t mq;
    struct mq_attr attr;
    char buffer[MAX_SIZE];
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(2, &mask); // Assign to CPU 0

    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) 
      {
         perror("sched_setaffinity (parent)");
         exit(EXIT_FAILURE);
      }

    // Set queue attributes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    // Open the queue for writing
    mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0644, &attr);

    if (mq == -1) 
    {
        perror("mq_open");
        exit(1);
    }

    // Prepare the message
    snprintf(buffer, sizeof(buffer), "Hello from writer!");






    return 0;
}
