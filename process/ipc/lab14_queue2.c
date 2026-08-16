#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define QUEUE_NAME  "/myqueue2"
#define MAX_SIZE    1024

int main()
 {
    mqd_t mq;
    char buffer[MAX_SIZE + 1];

    // Open the queue for reading
    mq = mq_open(QUEUE_NAME, O_RDONLY);

    if (mq == -1)
     {
        perror("mq_open");
        exit(1);
    }

    return 0;
}
