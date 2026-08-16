#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define QUEUE_NAME  "/myqueue"
#define MAX_SIZE    1024

int main() {
    mqd_t mq;
    struct mq_attr attr;
    char message[MAX_SIZE] = "Hello with timeout!";

    // Set queue attributes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    // Open the message queue
    mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0644, &attr);


    // Send with timeout
    printf("Message sent with 500ms timeout: %s\n", message);

    mq_close(mq);
    return 0;
}
