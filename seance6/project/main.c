/*
 * ============================================================================
 * TRAINING EXAMPLE - Embedded C multithreaded telemetry server
 * ----------------------------------------------------------------------------
 * Threads:
 *   1) monitor_thread : watches config.json with inotify, parses it with
 *      cJSON, extracts "sw_version" and "period", pushes updates to the
 *      server thread through a shared struct + eventfd notification.
 *   2) server_thread   : single epoll loop multiplexing:
 *          - TCP listening socket (Python client connects here)
 *          - one or more connected TCP client sockets
 *          - a named FIFO (local IPC, e.g. from a shell or another process)
 *          - an eventfd used by monitor_thread to signal "config changed"
 *          - a timerfd used to send periodic telemetry
 *
 * Build:
 *   gcc -Wall -Wextra -o server main.c cJSON.c -lpthread -lrt
 *
 * Run:
 *   ./server
 *   (in another terminal) echo hello > /tmp/telemetry_fifo
 *   python3 client_example.py     (see comment at bottom of file)
 *
 * NOTE: this is intentionally kept in ONE file and uses a single global
 * "app context" struct so trainees can read it top-to-bottom. In a real
 * project you would split it into config.c/.h, monitor.c/.h, server.c/.h.
 * ============================================================================
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <time.h>

#include "cJSON.h"

/* -------------------------------------------------------------------------
 * Configuration constants
 * ---------------------------------------------------------------------- */
#define CONFIG_PATH     "./config.json"
#define CONFIG_DIR      "."
#define CONFIG_FILE     "config.json"
#define FIFO_PATH       "/tmp/telemetry_fifo"
#define TCP_PORT        6000
#define MAX_CLIENTS     8
#define MAX_EPOLL_EVENTS 16

/* -------------------------------------------------------------------------
 * Shared configuration data, protected by a mutex.
 * monitor_thread writes it, server_thread reads it.
 * ---------------------------------------------------------------------- */
typedef struct {
    pthread_mutex_t lock;
    char sw_version[64];
    int  period_sec;
    int  valid;          /* 0 until first successful parse */
} shared_config_t;

static shared_config_t g_cfg = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .sw_version = "unknown",
    .period_sec = 5,
    .valid = 0
};

/* eventfd used by monitor_thread to say "I updated g_cfg, go re-read it" */
static int g_cfg_notify_fd = -1;

/* set by SIGINT handler, polled by both thread loops */
static volatile sig_atomic_t g_stop = 0;

static void on_sigint(int signo) {
    (void)signo;
    g_stop = 1;
}

/* -------------------------------------------------------------------------
 * Config parsing (JSON via cJSON). Swapping to XML later only means
 * rewriting this one function + linking mxml/libxml2 instead of cJSON -
 * the rest of the program (threads, epoll, sockets) does not change.
 * ---------------------------------------------------------------------- */
static int parse_config_file(const char *path, char *sw_version, size_t sw_len, int *period)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[monitor] cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return -1; }

    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return -1; }
    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        fprintf(stderr, "[monitor] JSON parse error near: %s\n", cJSON_GetErrorPtr());
        return -1;
    }

    const cJSON *ver = cJSON_GetObjectItemCaseSensitive(root, "sw_version");
    const cJSON *per = cJSON_GetObjectItemCaseSensitive(root, "period");

    int ok = 1;
    if (cJSON_IsString(ver) && ver->valuestring) {
        snprintf(sw_version, sw_len, "%s", ver->valuestring);
    } else {
        ok = 0;
    }
    if (cJSON_IsNumber(per)) {
        *period = per->valueint;
    } else {
        ok = 0;
    }

    cJSON_Delete(root);
    return ok ? 0 : -1;
}

/* =========================================================================
 * MONITOR THREAD
 * =========================================================================
 * Watches the DIRECTORY containing config.json (not the file itself).
 * This is the robust pattern: many editors/tools update a file by writing
 * a temp file then renaming it over the original, which replaces the
 * inode. Watching the file directly would silently lose the watch after
 * such a rename. Watching the directory and filtering by filename survives
 * both in-place writes and replace-by-rename.
 * ---------------------------------------------------------------------- */
static void *monitor_thread_fn(void *arg)
{
    (void)arg;
    int ino_fd = inotify_init1(IN_NONBLOCK);
    if (ino_fd < 0) { perror("inotify_init1"); return NULL; }

    int wd = inotify_add_watch(ino_fd, CONFIG_DIR,
                                IN_CLOSE_WRITE | IN_MOVED_TO | IN_DELETE | IN_MOVED_FROM);
    if (wd < 0) { perror("inotify_add_watch"); close(ino_fd); return NULL; }

    int epfd = epoll_create1(0);
    struct epoll_event ev = { .events = EPOLLIN, .data.fd = ino_fd };
    epoll_ctl(epfd, EPOLL_CTL_ADD, ino_fd, &ev);

    /* initial load so the server thread has valid values from t=0 */
    char version[64];
    int period;
    if (parse_config_file(CONFIG_PATH, version, sizeof(version), &period) == 0)
     {
        pthread_mutex_lock(&g_cfg.lock);
        snprintf(g_cfg.sw_version, sizeof(g_cfg.sw_version), "%s", version);
        g_cfg.period_sec = period;
        g_cfg.valid = 1;
        pthread_mutex_unlock(&g_cfg.lock);


        printf("[monitor] initial config: sw_version=%s period=%d\n", version, period);
        eventfd_write(g_cfg_notify_fd, 1);

    }
     else 
     {
        fprintf(stderr, "[monitor] no valid config at startup, using defaults\n");
    }

    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));

    while (!g_stop) 
    {
        struct epoll_event events[4];
        int n = epoll_wait(epfd, events, 4, 1000 /* ms, lets us check g_stop */);
        if (n < 0) 
        {
            if (errno == EINTR) continue;
            perror("epoll_wait(monitor)");
            break;
        }
        
        for (int i = 0; i < n; i++) 
        {
            if (events[i].data.fd != ino_fd) continue;

            ssize_t len = read(ino_fd, buf, sizeof(buf));
            if (len <= 0) continue;

            for (char *p = buf; p < buf + len; )
             {
                struct inotify_event *e = (struct inotify_event *)p;
                if (e->len && strcmp(e->name, CONFIG_FILE) == 0)
                 {
                    if (e->mask & (IN_DELETE | IN_MOVED_FROM)) 
                    {
                        fprintf(stderr, "[monitor] %s removed - keeping last known config\n",
                                CONFIG_FILE);
                    }
                     else 
                     { /* IN_CLOSE_WRITE or IN_MOVED_TO */
                        if (parse_config_file(CONFIG_PATH, version, sizeof(version), &period) == 0) 
                        {
                            pthread_mutex_lock(&g_cfg.lock);
                            int changed = (period != g_cfg.period_sec) ||
                                          strcmp(version, g_cfg.sw_version) != 0;
                            snprintf(g_cfg.sw_version, sizeof(g_cfg.sw_version), "%s", version);
                            g_cfg.period_sec = period;
                            g_cfg.valid = 1;
                            pthread_mutex_unlock(&g_cfg.lock);

                            if (changed) 
                            {
                                printf("[monitor] config updated: sw_version=%s period=%d\n",
                                       version, period);
                                eventfd_write(g_cfg_notify_fd, 1); /* wake server thread */
                            }
                        } 
                        else 
                        {
                            fprintf(stderr, "[monitor] config rewritten but invalid, ignoring\n");
                        }
                    }
                }
                p += sizeof(struct inotify_event) + e->len;
            }
        }
    }

    close(epfd);
    inotify_rm_watch(ino_fd, wd);
    close(ino_fd);
    printf("[monitor] stopped\n");
    return NULL;
}

/* =========================================================================
 * SERVER THREAD
 * ========================================================================= */
typedef struct {
    int fd;
    int used;
} client_t;

static int make_listen_socket(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) { perror("socket"); return -1; }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return -1;
    }
    if (listen(fd, 8) < 0) {
        perror("listen"); close(fd); return -1;
    }
    return fd;
}

static int open_fifo(const char *path)
{
    unlink(path); /* fresh FIFO on each run, ignore error if absent */
    if (mkfifo(path, 0666) < 0 && errno != EEXIST) {
        perror("mkfifo"); return -1;
    }
    /* Opening a FIFO O_RDONLY blocks until a writer appears, and once the
     * writer closes, epoll would keep reporting EPOLLIN/EPOLLHUP forever
     * (read returns 0 = EOF) unless we reopen it. The common workaround
     * used in embedded Linux code is to open our own read/write end so
     * the FIFO always has at least one writer (ourselves) and never
     * shows EOF. We never write real data from this end, only read
     * whatever real external writers send. */
    int fd = open(path, O_RDWR | O_NONBLOCK);
    if (fd < 0) { perror("open fifo"); return -1; }
    return fd;
}

static void add_client(client_t clients[], int max, int fd)
{
    for (int i = 0; i < max; i++) {
        if (!clients[i].used) {
            clients[i].used = 1;
            clients[i].fd = fd;
            return;
        }
    }
    fprintf(stderr, "[server] client table full, rejecting fd=%d\n", fd);
    close(fd);
}

static void remove_client(client_t clients[], int max, int epfd, int fd)
{
    for (int i = 0; i < max; i++) {
        if (clients[i].used && clients[i].fd == fd) {
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            clients[i].used = 0;
            return;
        }
    }
}

static void broadcast_telemetry(client_t clients[], int max, const char *sw_version, int period)
{
    static uint64_t sample_id = 0;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    char msg[256];
    int len = snprintf(msg, sizeof(msg),
        "{\"type\":\"telemetry\",\"id\":%llu,\"ts\":%ld,"
        "\"sw_version\":\"%s\",\"period\":%d}\n",
        (unsigned long long)sample_id++, (long)ts.tv_sec, sw_version, period);

    for (int i = 0; i < max; i++) {
        if (clients[i].used) {
            ssize_t w = write(clients[i].fd, msg, (size_t)len);
            if (w < 0 && errno != EAGAIN) {
                /* client gone, will be cleaned up on next EPOLLHUP/EPOLLIN=0 */
                perror("write telemetry");
            }
        }
    }
}

static void *server_thread_fn(void *arg)
{
    (void)arg;

    int listen_fd = make_listen_socket(TCP_PORT);
    if (listen_fd < 0) return NULL;
    printf("[server] listening on TCP port %d\n", TCP_PORT);

    int fifo_fd = open_fifo(FIFO_PATH);
    if (fifo_fd < 0) { close(listen_fd); return NULL; }
    printf("[server] FIFO ready at %s\n", FIFO_PATH);

    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timer_fd < 0) { perror("timerfd_create"); return NULL; }

    pthread_mutex_lock(&g_cfg.lock);
    int period = g_cfg.period_sec;
    char sw_version[64];
    snprintf(sw_version, sizeof(sw_version), "%s", g_cfg.sw_version);
    pthread_mutex_unlock(&g_cfg.lock);

    struct itimerspec its = {
        .it_value    = { .tv_sec = period, .tv_nsec = 0 },
        .it_interval = { .tv_sec = period, .tv_nsec = 0 }
    };
    timerfd_settime(timer_fd, 0, &its, NULL);

    int epfd = epoll_create1(0);
    struct epoll_event ev;

    ev.events = EPOLLIN; ev.data.fd = listen_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    ev.events = EPOLLIN; ev.data.fd = fifo_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fifo_fd, &ev);

    ev.events = EPOLLIN; ev.data.fd = timer_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, timer_fd, &ev);

    ev.events = EPOLLIN; ev.data.fd = g_cfg_notify_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, g_cfg_notify_fd, &ev);

    client_t clients[MAX_CLIENTS] = {0};

    while (!g_stop) 
    {
        struct epoll_event events[MAX_EPOLL_EVENTS];
        int n = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, 1000);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait(server)");
            break;
        }

        for (int i = 0; i < n; i++) 
        {
            int fd = events[i].data.fd;
            uint32_t evs = events[i].events;

            if (fd == listen_fd) 
            {
                for (;;)
                 {
                    struct sockaddr_in peer;
                    socklen_t plen = sizeof(peer);
                    int cfd = accept4(listen_fd, (struct sockaddr *)&peer, &plen, SOCK_NONBLOCK);
                    if (cfd < 0) 
                    {
                        if (errno != EAGAIN && errno != EWOULDBLOCK)
                            perror("accept4");
                        break; /* no more pending connections */
                    }
                    printf("[server] client connected from %s:%d\n",
                           inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));

                    struct epoll_event cev = { .events = EPOLLIN, .data.fd = cfd };

                    epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &cev);

                    add_client(clients, MAX_CLIENTS, cfd);

                }

            } 
            else if (fd == fifo_fd) 
            {
                char buf[256];
                ssize_t r;
                while ((r = read(fifo_fd, buf, sizeof(buf) - 1)) > 0) 
                {
                    buf[r] = '\0';
                    printf("[server] FIFO message: %s", buf);
                }

            } 
            else if (fd == timer_fd) 
            {
                uint64_t expirations;
                if (read(timer_fd, &expirations, sizeof(expirations)) > 0) 
                {
                    pthread_mutex_lock(&g_cfg.lock);
                    snprintf(sw_version, sizeof(sw_version), "%s", g_cfg.sw_version);
                    pthread_mutex_unlock(&g_cfg.lock);
                    broadcast_telemetry(clients, MAX_CLIENTS, sw_version, period);
                }

            } 
            else if (fd == g_cfg_notify_fd) 
            {
                uint64_t val;
                read(g_cfg_notify_fd, &val, sizeof(val));
                pthread_mutex_lock(&g_cfg.lock);
                period = g_cfg.period_sec;
                snprintf(sw_version, sizeof(sw_version), "%s", g_cfg.sw_version);
                pthread_mutex_unlock(&g_cfg.lock);
                printf("[server] applying new period=%d sw_version=%s\n", period, sw_version);
                its.it_value.tv_sec = period;
                its.it_interval.tv_sec = period;
                timerfd_settime(timer_fd, 0, &its, NULL);

            }
             else 
             {
                /* one of the connected TCP clients */
                if (evs & (EPOLLHUP | EPOLLERR)) 
                {
                    remove_client(clients, MAX_CLIENTS, epfd, fd);
                    continue;
                }
                
                char buf[256];
                ssize_t r = read(fd, buf, sizeof(buf) - 1);
                if (r <= 0) 
                {
                    if (r == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) 
                    {
                        printf("[server] client fd=%d disconnected\n", fd);
                        remove_client(clients, MAX_CLIENTS, epfd, fd);
                    }
                } 
                else 
                {
                    buf[r] = '\0';
                    printf("[server] client fd=%d says: %s\n", fd, buf);
                }
            }
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].used) close(clients[i].fd);
        
    close(timer_fd);
    close(fifo_fd);
    close(listen_fd);
    close(epfd);
    unlink(FIFO_PATH);
    printf("[server] stopped\n");
    return NULL;
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(void)
{
    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);
    signal(SIGPIPE, SIG_IGN); /* writing to a closed socket must not kill us */

    g_cfg_notify_fd = eventfd(0, EFD_NONBLOCK);
    if (g_cfg_notify_fd < 0) { perror("eventfd"); return 1; }

    pthread_t th_monitor, th_server;
    pthread_create(&th_monitor, NULL, monitor_thread_fn, NULL);
    pthread_create(&th_server, NULL, server_thread_fn, NULL);

    pthread_join(th_monitor, NULL);
    pthread_join(th_server, NULL);

    close(g_cfg_notify_fd);
    return 0;
}

/* ============================================================================
 * Minimal Python client for testing (save separately as client_example.py):
 *
 * import socket
 * s = socket.create_connection(("127.0.0.1", 6000))
 * while True:
 *     data = s.recv(4096)
 *     if not data:
 *         break
 *     print(data.decode().strip())
 * ============================================================================
 */
