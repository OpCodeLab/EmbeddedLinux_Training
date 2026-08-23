/*
 * ipc_server.c -- Educational IPC background server
 * ==================================================
 * Companion C program for the PyQt5 GUI (gui.py). Demonstrates four
 * classic Linux IPC mechanisms side by side so students can compare them:
 *
 *   1. A signal              -- SIGUSR1, sent from the GUI via os.kill()
 *   2. A POSIX message queue -- /edu_ipc_mq
 *   3. A Unix domain socket  -- /tmp/edu_ipc.sock
 *   4. POSIX shared memory   -- /edu_ipc_shm, guarded by semaphore /edu_ipc_sem
 *
 * Once per second this program builds ONE message (with a sequence number
 * and a CLOCK_MONOTONIC timestamp) and delivers the identical payload
 * through the message queue, broadcasts it to every connected socket
 * client, and writes it into shared memory. Because all three channels
 * carry the same seq/timestamp, students can directly compare arrival
 * latency and reliability across mechanisms in the GUI.
 *
 * Build:
 *     make
 * Run:
 *     ./ipc_server
 * Then copy the printed PID into the GUI's "Target PID" field and click
 * "Send SIGUSR1" -- the sig_count field in every subsequent message will
 * increment.
 *
 * Stop with Ctrl+C (SIGINT); the program cleans up all IPC objects on exit.
 */

/* Expose POSIX.1-2008 declarations (siginfo_t, CLOCK_MONOTONIC, sigaction(),
 * ftruncate(), usleep(), ...) under -std=c11's strict mode. Must come before
 * any system header is included. */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>

#include <mqueue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#include <sys/socket.h>
#include <sys/un.h>

#define MQ_NAME     "/edu_ipc_mq"
#define SHM_NAME    "/edu_ipc_shm"
#define SEM_NAME    "/edu_ipc_sem"
#define SOCK_PATH   "/tmp/edu_ipc.sock"
#define MAX_CLIENTS 8
#define TICK_MS     1000

/*
 * Wire format shared between C and Python. #pragma pack(1) removes any
 * compiler-inserted padding so the byte layout is fully deterministic --
 * the Python side mirrors this exactly with struct.Struct("<Idi64s").
 */
#pragma pack(push, 1)
typedef struct {
    uint32_t seq;         /* message sequence number, increments every tick */
    double   timestamp;   /* CLOCK_MONOTONIC seconds at send time */
    int32_t  sig_count;   /* number of SIGUSR1 received so far */
    char     text[64];    /* null-terminated human-readable payload */
} ipc_msg_t;
#pragma pack(pop)


static volatile sig_atomic_t g_sigusr1_count = 0;
static volatile sig_atomic_t g_running = 1;

static void handle_sigusr1(int sig, siginfo_t *si, void *unused) {
    (void)sig; (void)si; (void)unused;
    g_sigusr1_count++;
}

static void handle_terminate(int sig) {
    (void)sig;
    g_running = 0;
}

static double now_monotonic(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(void) 
{
    printf("=== IPC Server ===\n");
    printf("PID: %d\n", getpid());
    printf("Send SIGUSR1 to this PID from the GUI to bump the signal counter.\n\n");
    fflush(stdout);

    /* ---- signal handling (see the SIGSEGV/SIGUSR1 discussion this demo builds on) ---- */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handle_sigusr1;
    sa.sa_flags = SA_SIGINFO;
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1) 
     {
         perror("sigaction"); exit(1); 
    }
    signal(SIGINT, handle_terminate);
    signal(SIGTERM, handle_terminate);
    signal(SIGPIPE, SIG_IGN); /* a socket client disconnecting must not kill us */

    /* ---- POSIX message queue ---- */
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg  = 10;
    attr.mq_msgsize = sizeof(ipc_msg_t);

    mq_unlink(MQ_NAME); /* drop any stale queue left over from a previous run */
    
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY | O_NONBLOCK, 0666, &attr);
    
    if (mq == (mqd_t)-1) 
    {
        perror("mq_open");

        fprintf(stderr, "Hint: on WSL, POSIX message queues may need: "
                        "sudo mkdir -p /dev/mqueue && sudo mount -t mqueue none /dev/mqueue\n");
        exit(1);
    }

    /* ---- POSIX shared memory, guarded by a named semaphore ---- */
    shm_unlink(SHM_NAME);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

    if (shm_fd == -1) 
    { perror("shm_open"); exit(1); 
    }

    if (ftruncate(shm_fd, sizeof(ipc_msg_t)) == -1) 
    { 
        perror("ftruncate"); exit(1); 
    }

    ipc_msg_t *shm_ptr = mmap(NULL, sizeof(ipc_msg_t), PROT_READ | PROT_WRITE,
                              MAP_SHARED, shm_fd, 0);

    if (shm_ptr == MAP_FAILED)     
    {
         perror("mmap"); exit(1); 
    }

    memset(shm_ptr, 0, sizeof(ipc_msg_t));

    sem_unlink(SEM_NAME);
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1); /* binary mutex, starts unlocked */
    if (sem == SEM_FAILED) 
    {
         perror("sem_open"); 
         exit(1); 
    }

    /* ---- Unix domain socket server ---- */
    unlink(SOCK_PATH);

    int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (listen_fd == -1)
     { 
        perror("socket"); exit(1); 
    }

    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) 
    {
        perror("bind"); 
        exit(1);
    }

    if (listen(listen_fd, MAX_CLIENTS) == -1)
     { 
        perror("listen"); 
        exit(1); 
    }

    fcntl(listen_fd, F_SETFL, fcntl(listen_fd, F_GETFL, 0) | O_NONBLOCK);

    int clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;

    printf("Message queue : %s\n", MQ_NAME);
    printf("Shared memory : %s (semaphore %s)\n", SHM_NAME, SEM_NAME);
    printf("Socket        : %s\n", SOCK_PATH);
    printf("sizeof(ipc_msg_t) = %zu bytes\n", sizeof(ipc_msg_t));
    printf("Broadcasting one message per second on all three channels...\n\n");
    fflush(stdout);

    uint32_t seq = 0;

    while (g_running) 
    {
        /* accept any newly-connecting client, non-blocking */
        int c = accept(listen_fd, NULL, NULL);
        if (c != -1) 
        {
            fcntl(c, F_SETFL, O_NONBLOCK);
            int slot = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) 
            {
                if (clients[i] == -1) 
                { 
                    slot = i;
                     break; 
                    }
            }
            if (slot != -1) 
            {
                clients[slot] = c;
                printf("[socket] client connected (fd=%d)\n", c);
            } 
            else 
            {
                close(c); /* no room left */
            }
        }

        /* build this tick's shared payload */
        seq++;
        ipc_msg_t msg;
        memset(&msg, 0, sizeof(msg));
        msg.seq       = seq;
        msg.timestamp = now_monotonic();
        msg.sig_count = g_sigusr1_count;
        snprintf(msg.text, sizeof(msg.text), "tick #%u (SIGUSR1 x%d)", seq, g_sigusr1_count);

        /* 1) message queue */
        if (mq_send(mq, (const char *)&msg, sizeof(msg), 0) == -1 && errno != EAGAIN) 
        {
            perror("mq_send");
        }

        /* 2) socket: broadcast to every connected client */
        for (int i = 0; i < MAX_CLIENTS; i++) 
        {
            if (clients[i] != -1) 
            {
                ssize_t n = send(clients[i], &msg, sizeof(msg), MSG_NOSIGNAL);
                if (n == -1)
                 {
                    close(clients[i]);
                    clients[i] = -1;
                }
            }
        }

        /* 3) shared memory: writer holds the semaphore while copying */
        sem_wait(sem);
        memcpy(shm_ptr, &msg, sizeof(msg));
        sem_post(sem);

        printf("[tick %u] sig_count=%d text=\"%s\"\n", seq, g_sigusr1_count, msg.text);
        fflush(stdout);

        struct timespec tick = { .tv_sec = TICK_MS / 1000,
                                  .tv_nsec = (TICK_MS % 1000) * 1000000L };
        nanosleep(&tick, NULL); /* usleep() is obsolete under strict POSIX.1-2008 */
    }

    printf("\nShutting down, cleaning up IPC objects...\n");
    mq_close(mq);
    mq_unlink(MQ_NAME);
    munmap(shm_ptr, sizeof(ipc_msg_t));
    close(shm_fd);
    shm_unlink(SHM_NAME);
    sem_close(sem);
    sem_unlink(SEM_NAME);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1) close(clients[i]);
    }
    close(listen_fd);
    unlink(SOCK_PATH);
    return 0;
}