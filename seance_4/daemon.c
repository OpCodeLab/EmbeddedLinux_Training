/*
 * mydaemon.c - minimal Linux daemon that writes a periodic message to syslog
 *
 * Build:   gcc -Wall -O2 -o mydaemon mydaemon.c
 * Run:     ./mydaemon [interval_seconds]     (default interval: 10s)
 * Stop:    kill <pid>          (sends SIGTERM -> clean shutdown)
 * Logs:    journalctl -t mydaemon -f
 *      or  tail -f /var/log/syslog   (Debian/Ubuntu)
 *      or  tail -f /var/log/messages (RHEL/CentOS)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>

#define NR_OPEN 1024   /* fallback if sysconf() is unavailable */

static volatile sig_atomic_t keep_running = 1;

/* SIGTERM handler: just flip a flag, do the real work in main() */
static void handle_sigterm (int sig)
{
    (void) sig;
    keep_running = 0;
}

/*
 * daemonize() - classic double-fork daemonization
 *
 * Steps (based on the standard recipe, e.g. Stevens/TLPI):
 *   1. fork() + parent exit           -> child is no longer a process group leader
 *   2. setsid()                       -> new session, new process group, no controlling tty
 *   3. fork() again + parent exit     -> daemon can never re-acquire a controlling tty
 *   4. chdir("/")                     -> don't hold any directory busy
 *   5. umask(0)                       -> don't inherit a restrictive umask
 *   6. close all open file descriptors
 *   7. redirect fd 0,1,2 to /dev/null
 */
static void daemonize (void)
{
    pid_t pid;
    int fd, maxfd;

    /* --- first fork --- */
    pid = fork ();
    if (pid < 0)
        exit (EXIT_FAILURE);
    if (pid > 0)
        exit (EXIT_SUCCESS);          /* parent terminates */

    /* --- create new session and process group --- */
    if (setsid () == -1)
        exit (EXIT_FAILURE);

    /* ignore SIGHUP, which we'd otherwise get when the session leader exits */
    signal (SIGHUP, SIG_IGN);

    /* --- second fork: guarantees we are not a session leader,
       so we can never accidentally acquire a controlling terminal --- */
    pid = fork ();
    if (pid < 0)
        exit (EXIT_FAILURE);
    if (pid > 0)
        exit (EXIT_SUCCESS);          /* first child terminates */

    /* --- set the working directory to the root directory --- */
    if (chdir ("/") == -1)
        exit (EXIT_FAILURE);

    /* --- clear file mode creation mask --- */
    umask (0);

    /* --- close all open files --- */
    maxfd = (int) sysconf (_SC_OPEN_MAX);
    if (maxfd == -1)
        maxfd = NR_OPEN;              /* fallback if the limit is indeterminate */
    for (fd = 0; fd < maxfd; fd++)
        close (fd);

    /* --- redirect fd's 0, 1, 2 to /dev/null --- */
    fd = open ("/dev/null", O_RDWR);  /* stdin  -> fd 0 */
    if (dup (fd) == -1)                /* stdout -> fd 1 */
        exit (EXIT_FAILURE);
    if (dup (fd) == -1)                /* stderr -> fd 2 */
        exit (EXIT_FAILURE);
}

int
main (int argc, char *argv[])
{
    int interval = 10;   /* seconds between log messages */

    if (argc > 1)
        interval = atoi (argv[1]);
    if (interval <= 0)
        interval = 10;

    daemonize ();

    /* handle termination signals cleanly once we're a daemon */
    signal (SIGTERM, handle_sigterm);
    signal (SIGINT, handle_sigterm);   /* harmless once detached, but tidy */

    /* open the connection to syslog
     *   ident   : tag shown in each log line
     *   LOG_PID : include our pid in each message
     *   LOG_CONS: also write to console if syslog itself is unreachable
     *   LOG_DAEMON: standard facility for system daemons
     */
    openlog ("mydaemon", LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog (LOG_NOTICE, "daemon started, pid=%d, interval=%ds", getpid (), interval);

    while (keep_running)
    {
        syslog (LOG_INFO, "heartbeat: still alive (pid=%d)", getpid ());
        sleep (interval);            /* interrupted early if a signal arrives */
    }

    syslog (LOG_NOTICE, "daemon stopping, pid=%d", getpid ());
    closelog ();

    return EXIT_SUCCESS;
}