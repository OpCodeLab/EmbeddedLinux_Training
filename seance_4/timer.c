/*
 * timer_create_timer.c — POSIX per-process timer via timer_create()/timer_settime().
 *
 * Same schedule as alarm_timer.c: 6s x3, then 2s x3, then disarmed and destroyed.
 * Unlike alarm(), this timer is its own object — a process can have many of
 * these, each independent of alarm()/setitimer().
 *
 * Build: gcc -O2 -Wall -o timer_create_timer timer_create_timer.c -lrt
 * Run:   ./timer_create_timer
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#define TICK_SIGNAL SIGRTMIN

static volatile sig_atomic_t ticks = 0;
static timer_t timerid;

static void arm(int seconds)
 {
    struct itimerspec its = {
        .it_value    = { .tv_sec = seconds },
        .it_interval = { 0 },               /* one-shot; the handler re-arms explicitly */
    };

    timer_settime(timerid, 0, &its, NULL);
}

/* SA_SIGINFO handler — same async-signal-safety rules as a plain signal handler. */
static void on_tick(int signum, siginfo_t *si, void *uctx)
 {
    (void)signum; (void)si; (void)uctx;
    ticks++;
    ssize_t r = write(STDOUT_FILENO, "[timer_create] tick\n", 20);
    (void)r;

    if (ticks < 6)
        arm(ticks < 3 ? 6 : 2);             /* MODIFY: re-arm, faster after tick 3 */
}

int main(void) 
{
    struct sigaction sa = { .sa_flags = SA_SIGINFO, .sa_sigaction = on_tick };
    sigemptyset(&sa.sa_mask);
    sigaction(TICK_SIGNAL, &sa, NULL);

    struct sigevent sev = {
        .sigev_notify = SIGEV_SIGNAL,
        .sigev_signo  = TICK_SIGNAL,
    };


    timer_create(CLOCK_MONOTONIC, &sev, &timerid);      /* CREATE: not armed yet */

    printf("[main] starting timer_create(): 6s x3, then 2s x3\n");
    arm(6);                                              /* START */

    while (ticks < 6)
        pause();                                         /* RUN: block until a signal arrives */

    struct itimerspec off = {0};
    timer_settime(timerid, 0, &off, NULL);                /* STOP: disarm, object still exists */
    timer_delete(timerid);                                /* DESTROY: object is gone */

    printf("[main] done, exiting\n");
    return 0;
}