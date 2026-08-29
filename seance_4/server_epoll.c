/*
 * epoll_timerfd_echo_server.c
 *
 * Minimal example combining:
 *   - a TCP listening socket (accepts client connections, echoes input)
 *   - a timerfd that fires every 6 seconds
 * all multiplexed on a single epoll instance / single thread.
 *
 * Build:  gcc -O2 -Wall -o echo_server epoll_timerfd_echo_server.c
 * Run:    ./echo_server 8080
 * Test:   nc 127.0.0.1 8080      (type lines, they get echoed back)
 *         watch the "[timer] tick" message print every 6s regardless
 *         of whether any client is connected.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define MAX_EVENTS   16
#define BACKLOG      16
#define BUF_SIZE     512

/* Create, bind, and listen on a TCP socket for the given port. */
static int make_listen_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    if (listen(fd, BACKLOG) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }
    return fd;
}

/* Create a timerfd that fires every `seconds`, starting after `seconds`. */
static int make_timer(int seconds) 
{
    int fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (fd < 0) { perror("timerfd_create"); exit(EXIT_FAILURE); }

    struct itimerspec its = {0};
    its.it_value.tv_sec    = seconds;   /* first expiration */
    its.it_interval.tv_sec = seconds;   /* repeat every `seconds` */

    if (timerfd_settime(fd, 0, &its, NULL) < 0) {
        perror("timerfd_settime"); exit(EXIT_FAILURE);
    }
    return fd;
}

/* Register an fd with epoll for EPOLLIN, tagging it with its own fd number. */
static void epoll_add(int epfd, int fd)
 {
    struct epoll_event ev = { .events = EPOLLIN, .data.fd = fd };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        perror("epoll_ctl ADD"); exit(EXIT_FAILURE);
    }
}

static void handle_new_connection(int epfd, int listen_fd) 
{
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);

    int client_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            perror("accept");
        return;
    }

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
    printf("[server] client connected: %s:%d (fd=%d)\n",
           ip, ntohs(peer.sin_port), client_fd);

    epoll_add(epfd, client_fd);
}

/* Returns 0 to keep the connection open, -1 if it should be closed. */
static int handle_client_data(int client_fd) 
{
    char buf[BUF_SIZE];
    ssize_t n = read(client_fd, buf, sizeof(buf));

    if (n == 0) 
    {
        printf("[server] client fd=%d closed connection\n", client_fd);
        return -1;
    }

    if (n < 0) 
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;               /* nothing to read right now */
        perror("read");
        return -1;
    }

    /* echo back exactly what was received */
    ssize_t off = 0;
    while (off < n) {
        ssize_t w = write(client_fd, buf + off, n - off);
        if (w < 0) {
            if (errno == EINTR) continue;
            perror("write");
            return -1;
        }
        off += w;
    }
    return 0;
}

static void handle_timer(int timer_fd) 
{
    uint64_t expirations;
    /* must be read to re-arm/clear the fd's readable state */
    ssize_t n = read(timer_fd, &expirations, sizeof(expirations));
    if (n != sizeof(expirations)) {
        if (n < 0) perror("read timerfd");
        return;
    }
    printf("[timer] tick (fired %llu time%s since last check)\n",
           (unsigned long long)expirations, expirations == 1 ? "" : "s");
}

int main(int argc, char *argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : 8080;

    int listen_fd = make_listen_socket(port);
    int timer_fd  = make_timer(6);          /* fires every 6 seconds */

    int epfd = epoll_create1(0);
    if (epfd < 0) { perror("epoll_create1"); exit(EXIT_FAILURE); }

    epoll_add(epfd, listen_fd);
    epoll_add(epfd, timer_fd);

    printf("[server] listening on port %d, timer every 6s (epfd=%d)\n",
           port, epfd);

    struct epoll_event events[MAX_EVENTS];
    for (;;) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);  /* block until ready */
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == listen_fd) {
                handle_new_connection(epfd, listen_fd);
            } else if (fd == timer_fd) {
                handle_timer(timer_fd);
            } else {
                if (handle_client_data(fd) < 0) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                }
            }
        }
    }

    close(epfd);
    close(timer_fd);
    close(listen_fd);
    return 0;
}