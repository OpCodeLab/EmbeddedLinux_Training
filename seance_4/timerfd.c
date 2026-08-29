/*
 * timerfd_demo.c
 *
 * Minimal demo of a periodic timer using timerfd_create/timerfd_settime.
 *
 * Behaviour:
 *   1. Create a timer that fires every 5 seconds.
 *   2. Let it fire 3 times (~15s), reading the fd each time.
 *   3. Change the period to 2 seconds (modify).
 *   4. Let it fire 3 more times with the new period.
 *   5. Stop the timer (disarm it) and show that read() then blocks
 *      forever (so we don't call read() again -- we just exit).
 *
 * Build:
 *   gcc -Wall -Wextra -o timerfd_demo timerfd_demo.c
 *
 * Run:
 *   ./timerfd_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/timerfd.h>

/* Set the timerfd's initial expiration + period.
 * initial_sec == 0 disarms (stops) the timer.
 */
static void set_timer(int tfd, long initial_sec, long period_sec)
{
    struct itimerspec new_value;

    memset(&new_value, 0, sizeof(new_value));
    new_value.it_value.tv_sec  = initial_sec;   /* delay before first fire */
    new_value.it_interval.tv_sec  = period_sec; /* 0 = one-shot, >0 = periodic */

    if (timerfd_settime(tfd, 0, &new_value, NULL) == -1) {
        perror("timerfd_settime");
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    /* CLOCK_MONOTONIC: not affected by wall-clock/date changes.
     * Use CLOCK_REALTIME if you need it tied to the system clock instead.
     */
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd == -1) 
    {
        perror("timerfd_create");
        exit(EXIT_FAILURE);
    }

    /* --- 1. Arm: fire first after 5s, then every 5s --- */
    printf("Starting periodic timer: every 5s\n");
    set_timer(tfd, 5, 5);

    uint64_t expirations;
    for (int i = 1; i <= 3; i++) 
    {
        ssize_t n = read(tfd, &expirations, sizeof(expirations));
        if (n != sizeof(expirations)) 
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
        /* expirations > 1 means the timer fired more than once
         * before we called read() (we were "late"). */
        printf("Tick %d (5s period), missed ticks = %llu\n",
               i, (unsigned long long)(expirations - 1));
    }

    /* --- 2. Modify: change the period to 2s while it's running --- */
    printf("\nChanging period to 2s\n");
    set_timer(tfd, 2, 2);

    for (int i = 1; i <= 3; i++) {
        ssize_t n = read(tfd, &expirations, sizeof(expirations));
        if (n != sizeof(expirations)) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        printf("Tick %d (2s period), missed ticks = %llu\n",
               i, (unsigned long long)(expirations - 1));
    }

    /* --- 3. Stop: disarm the timer (it_value all zero = stop) --- */
    printf("\nStopping timer\n");
    set_timer(tfd, 0, 0);
    printf("Timer stopped. No more ticks will be delivered.\n");

    close(tfd);
    return 0;
}