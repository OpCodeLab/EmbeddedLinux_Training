/*
 * Multi-threaded Process Synchronization + IPC demo
 * -------------------------------------------------
 * Parent process:
 *   - main thread : waitpid() on child
 *   - T1          : forwards terminal input to child via pipe_p2c_stdin (child's "stdin")
 *   - T2          : sends ping target to child via POSIX message queue
 *   - T3          : logger, reads ping output relayed from child via pipe_p2c
 *
 * Child process:
 *   - T1          : dup2's pipe_p2c_stdin read-end onto STDIN_FILENO, then scanf() reads it
 *   - T2          : mq_receive() waits for target host from parent
 *   - T3          : popen("ping -c N <host>") streams output into pipe_p2c
 *
 * Build: gcc -O0 -g -Wall -pthread ipc_project.c -lrt -o ipc_project
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <time.h>

#define MQ_NAME       "/ipc_demo_mq"
#define MQ_MAX_MSG    10
#define MQ_MSG_SIZE   256
#define PING_COUNT    4

/* pipe_p2c_stdin: parent -> child (acts as child's stdin)
 * pipe_p2c: child  -> parent (ping output relay)
 */
static int pipe_p2c_stdin[2]; /* pipe_p2c_stdin[0]=read (child), pipe_p2c_stdin[1]=write (parent) */

static int pipe_p2c[2]; /* pipe_p2c[0]=read (parent), pipe_p2c[1]=write (child) */

/* ------------------------- helpers ------------------------- */

static void log_msg(const char *tag, const char *msg) {
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    char ts[16];
    strftime(ts, sizeof(ts), "%H:%M:%S", &tm_now);
    printf("[%s][%s] %s\n", ts, tag, msg);
    fflush(stdout);
}

static void die(const char *ctx) {
    perror(ctx);
    exit(EXIT_FAILURE);
}

/* ===================== CHILD THREADS ===================== */

/* Child T1: redirect stdin to pipe_p2c_stdin read-end, then scanf reads whatever
 * the parent forwards from the terminal. NOTE: dup2 changes the fd table
 * for the WHOLE process, not just this thread — do it once, early,
 * before other threads rely on STDIN_FILENO. */
static void *child_thread1_stdin(void *arg) 
{
    (void)arg;
    if (dup2(pipe_p2c_stdin[0], STDIN_FILENO) < 0) die("child dup2 stdin");
    close(pipe_p2c_stdin[0]);
    close(pipe_p2c_stdin[1]); /* child doesn't write to pipe_p2c_stdin */

    char line[256];
    while (1) 
    {
        if (scanf("%255s", line) != 1)
           break;
        
           log_msg("child-T1", "got input via redirected stdin");
        
           if (strcmp(line, "quit") == 0) 
            break;
    }
    return NULL;
}

/* Child T2: waits on message queue for a target host from parent T2 */
static void *child_thread2_mq(void *arg) 
{
    char *target_out = (char *)arg; /* buffer shared with T3, size MQ_MSG_SIZE */
    mqd_t mq = mq_open(MQ_NAME, O_RDONLY);
    if (mq == (mqd_t)-1) die("child mq_open");

    char buf[MQ_MSG_SIZE + 1];
    ssize_t n = mq_receive(mq, buf, MQ_MSG_SIZE, NULL);
    if (n >= 0) 
    {
        buf[n] = '\0';
        strncpy(target_out, buf, MQ_MSG_SIZE - 1);
        log_msg("child-T2", "received ping target from parent");
    }
    mq_close(mq);
    return NULL;
}

/* Child T3: popen("ping ...") and stream lines into pipe_p2c -> parent logger */
static void *child_thread3_ping(void *arg)
 {
    char *target = (char *)arg;

    /* Wait until T2 has filled in the target (simple polling; a condvar
     * is a better classroom exercise for students to add). */
    while (target[0] == '\0') usleep(50 * 1000);

    close(pipe_p2c[0]); /* child doesn't read from pipe_p2c */

    char cmd[300];
    snprintf(cmd, sizeof(cmd), "ping -c %d %s", PING_COUNT, target);

    FILE *fp = popen(cmd, "r");
    if (!fp) { close(pipe_p2c[1]); return NULL; }

    char line[512];
    while (fgets(line, sizeof(line), fp))
     {
        write(pipe_p2c[1], line, strlen(line));
    }
    pclose(fp);

    close(pipe_p2c[1]); /* EOF signal to parent logger */
    return NULL;
}

static void run_child(void) 
{
    pthread_t t1, t2, t3;
    char target[MQ_MSG_SIZE] = {0};

    pthread_create(&t1, NULL, child_thread1_stdin, NULL);
    pthread_create(&t2, NULL, child_thread2_mq, target);
    pthread_create(&t3, NULL, child_thread3_ping, target);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    exit(EXIT_SUCCESS);
}

/* ===================== PARENT THREADS ===================== */

/* Parent T1: reads real terminal input, forwards it into pipe_p2c_stdin for child */
static void *parent_thread1_forward(void *arg) 
{
    (void)arg;
    close(pipe_p2c_stdin[0]); /* parent doesn't read from pipe_p2c_stdin */

    char line[256];
    
    while (fgets(line, sizeof(line), stdin))
     {
        write(pipe_p2c_stdin[1], line, strlen(line));
        
        if (strncmp(line, "quit", 4) == 0) 
          break;
    }

    close(pipe_p2c_stdin[1]);
    return NULL;
}

/* Parent T2: sends the ping target to child via mqueue */
static void *parent_thread2_mq(void *arg)
 {
    const char *target = (const char *)arg;
    
    mqd_t mq = mq_open(MQ_NAME, O_WRONLY);
   
    if (mq == (mqd_t)-1) 
      die("parent mq_open");

    if (mq_send(mq, target, strlen(target), 0) < 0)
      die("mq_send");
    
      log_msg("parent-T2", "sent ping target to child");

    mq_close(mq);
    return NULL;
}

/* Parent T3: logger — reads ping output relayed from child via pipe_p2c */
static void *parent_thread3_logger(void *arg)
 {
    (void)arg;
    close(pipe_p2c[1]); /* parent doesn't write to pipe_p2c */

    char buf[512];
    ssize_t n;
    while ((n = read(pipe_p2c[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        /* buf may contain a partial line; fine for a demo, students can
         * add proper line buffering as an exercise */
        printf("[ping] %s", buf);
        fflush(stdout);
    }
    close(pipe_p2c[0]);
    return NULL;
}

static void run_parent(pid_t child_pid, const char *ping_target) 
{
    pthread_t t1, t3;
    pthread_t t2;

    pthread_create(&t1, NULL, parent_thread1_forward, NULL);
    pthread_create(&t3, NULL, parent_thread3_logger, NULL);
    pthread_create(&t2, NULL, parent_thread2_mq, (void *)ping_target);

    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t1, NULL);

    int status;
    waitpid(child_pid, &status, 0);
    log_msg("parent-main", "child reaped");
}

/* ===================== MAIN ===================== */

int main(int argc, char **argv) 
{
    const char *ping_target = (argc > 1) ? argv[1] : "8.8.8.8";

    /* Create mqueue up front so both sides can open it safely */
    mq_unlink(MQ_NAME); /* clean any stale queue */

    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = MQ_MAX_MSG,
        .mq_msgsize = MQ_MSG_SIZE,
        .mq_curmsgs = 0
    };
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0644, &attr);
    
    if (mq == (mqd_t)-1) 
      die("mq_open create");
    
      mq_close(mq);

    if (pipe(pipe_p2c_stdin) < 0) 
      die("pipe_p2c_stdin");

    if (pipe(pipe_p2c) < 0) 
        die("pipe_p2c");

    pid_t pid = fork();
    
    if (pid < 0) 
          die("fork");

    if (pid == 0)
     {
        /* CHILD: only the forking thread survives fork(); we spawn
         * fresh threads here, not the parent's. */
        run_child();
    } 
    else 
    {
        run_parent(pid, ping_target);
        mq_unlink(MQ_NAME);
    }

    return 0;
}
