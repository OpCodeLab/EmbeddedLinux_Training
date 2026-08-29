/*
 * alarm_timer.c — the simplest Linux timer: alarm() + SIGALRM.
 *
 * Schedule: fires every 6s for the first 3 ticks, then every 2s for 3 more
 * (that re-arm is the "modify" step), then stops and the program exits.
 *
 * Build: gcc -O2 -Wall -o alarm_timer alarm_timer.c
 * Run:   ./alarm_timer
 */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t ticks = 0;

/* Signal handlers may only call async-signal-safe functions — write() and
 * alarm() are safe, printf() is not. */
static void on_alarm(int signum)
 {
    (void)signum;
    ticks++;
    ssize_t r = write(STDOUT_FILENO, "[alarm] tick\n", 13);
    (void)r;

    if (ticks < 6)
        alarm(2);   /* MODIFY: re-arm, faster after tick 3 */
    /* else: no re-arm — the timer is now stopped */
}

int main(void) {
    signal(SIGALRM, on_alarm);

    printf("[main] starting alarm(): 6s x3, then 2s x3\n");
    alarm(6);                       /* START */

    while (ticks < 6)
        pause();                    /* RUN: block until a signal arrives */

    alarm(0);                       /* STOP: cancel any pending alarm */
    printf("[main] done, exiting\n");
    return 0;
}