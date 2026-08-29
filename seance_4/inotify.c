/*
 * inotify_watch.c
 *
 * Watches a single file and prints an event when it is written to
 * or removed.
 *
 * Build:  gcc -Wall -o watch inotify_watch.c
 * Run:    ./watch /path/to/file
 *
 * Try it: in another terminal, `echo hi >> /path/to/file` or `rm /path/to/file`
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))   /* room for several events + names */

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <file-to-watch>\n", argv[0]);
        return 1;
    }

    int fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return 1;
    }

    /* IN_MODIFY        - fires on every write() into the file (can be many times)
     * IN_CLOSE_WRITE    - fires once, when a writer closes the fd (write is "done")
     * IN_DELETE_SELF    - the watched file itself was removed
     * IN_MOVE_SELF      - the watched file itself was renamed/moved away
     */
    int wd = inotify_add_watch(fd, argv[1],
                                IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF);
    if (wd < 0) 
    {
        perror("inotify_add_watch");
        close(fd);
        return 1;
    }

    printf("watching %s ...\n", argv[1]);

    char buf[BUF_LEN];

    for (;;) 
    {
        ssize_t len = read(fd, buf, BUF_LEN);   /* blocks until an event arrives */
        
        if (len < 0)
         {
            if (errno == EINTR)
                continue;
            perror("read");
            break;
        }

        for (char *ptr = buf; ptr < buf + len; ) 
        {
            struct inotify_event *event = (struct inotify_event *) ptr;

            if (event->mask & IN_MODIFY)
                printf("-> modified (write in progress)\n");
            if (event->mask & IN_CLOSE_WRITE)
                printf("-> write finished (writer closed the file)\n");
            if (event->mask & IN_DELETE_SELF)
                printf("-> file was deleted\n");
            if (event->mask & IN_MOVE_SELF)
                printf("-> file was moved/renamed away\n");
            if (event->mask & IN_IGNORED) 
            {
                /* kernel drops the watch automatically once the file is gone */
                printf("watch is no longer valid, exiting\n");
                close(fd);
                return 0;
            }

            ptr += EVENT_SIZE + event->len;
        }
    }

    close(fd);
    return 1;
}