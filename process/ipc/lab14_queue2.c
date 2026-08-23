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

  
    // Receive the message
    ssize_t bytes_read = mq_receive(mq, buffer, MAX_SIZE, NULL);
    if (bytes_read < 0)
     {
        perror("mq_receive");
        exit(1);
    }

    buffer[bytes_read] = '\0';
    printf("Received message: %s\n", buffer);

    mq_close(mq);
    mq_unlink(QUEUE_NAME); // Delete the queue

    return 0;
}
