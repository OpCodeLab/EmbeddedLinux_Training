/*
 * echo_client.c — simple test client for epoll_timerfd_echo_server.c
 *
 * Connects to the echo server, sends whatever you type (one line at a
 * time), and prints back what the server echoes.
 *
 * Build: gcc -O2 -Wall -o echo_client echo_client.c
 * Run:   ./echo_client                   # connects to 127.0.0.1:8080
 *        ./echo_client 192.168.1.10 9000 # or a specific host/port
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 512

static int connect_to(const char *ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "bad address: %s\n", ip);
        exit(EXIT_FAILURE);
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    return fd;
}

/* write() can send fewer bytes than asked; loop until it is all out. */
static int send_all(int fd, const char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, buf + off, len - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("write");
            return -1;
        }
        off += (size_t)n;
    }
    return 0;
}

int main(int argc, char *argv[]) 
{
    const char *ip   = (argc > 1) ? argv[1] : "127.0.0.1";
    int         port = (argc > 2) ? atoi(argv[2]) : 8080;

    int fd = connect_to(ip, port);
    fprintf(stderr, "[client] connected to %s:%d - type a line and press enter (Ctrl-D to quit)\n", ip, port);

    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        size_t len = strlen(line);

        if (send_all(fd, line, len) < 0)
            break;

        char reply[BUF_SIZE];
        ssize_t n = read(fd, reply, sizeof(reply) - 1);
        if (n == 0) {
            fprintf(stderr, "[client] server closed the connection\n");
            break;
        }
        if (n < 0) {
            perror("read");
            break;
        }
        reply[n] = '\0';
        printf("[server echoed] %s", reply);
        fflush(stdout);
    }

    close(fd);
    return 0;
}