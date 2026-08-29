#include <sys/times.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#define GN_GT_0 0x01   /* value must be > 0 */

static void errExit(const char *msg);
static int  getInt(const char *arg, int flags, const char *name);
static void displayProcessTimes(const char *msg);

/* ---- minimal replacements for TLPI helper functions ---- */

static void errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static int getInt(const char *arg, int flags, const char *name)
{
    char *endptr;
    long val;

    errno = 0;
    val = strtol(arg, &endptr, 10);

    if (errno != 0 || endptr == arg || *endptr != '\0') {
        fprintf(stderr, "%s must be an integer\n", name);
        exit(EXIT_FAILURE);
    }
    if ((flags & GN_GT_0) && val <= 0) {
        fprintf(stderr, "%s must be > 0\n", name);
        exit(EXIT_FAILURE);
    }
    return (int) val;
}




static void displayProcessTimes(const char *msg)
{
    struct timespec ts;
    struct rusage ru;

    if (msg != NULL)
        printf("%s", msg);

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
        errExit("clock_gettime");

    printf("wall time: %ld.%09ld sec\n",
           (long) ts.tv_sec, ts.tv_nsec);

    if (getrusage(RUSAGE_SELF, &ru) == -1)
        errExit("getrusage");

    printf("user CPU:  %ld.%06ld sec\n",
           (long) ru.ru_utime.tv_sec, (long) ru.ru_utime.tv_usec);
    printf("sys  CPU:  %ld.%06ld sec\n",
           (long) ru.ru_stime.tv_sec, (long) ru.ru_stime.tv_usec);
}




uint64_t sink;  /* used to prevent compiler from optimizing away loop */
int main(int argc, char *argv[])
{
    int numCalls, j;

    printf("CLOCKS_PER_SEC=%ld  sysconf(_SC_CLK_TCK)=%ld\n\n",
           (long) CLOCKS_PER_SEC, sysconf(_SC_CLK_TCK));

    //displayProcessTimes("At program start:\n");

    numCalls = (argc > 1) ? getInt(argv[1], GN_GT_0, "num-calls") : 100000000;

    for (j = 0; j < numCalls; j++)
        //no system call ;
        // getppid();  /* system call to get parent process ID */
        sink += j * j;
    //displayProcessTimes("After getppid() loop:\n");
    

    sleep(2);
    exit(EXIT_SUCCESS);
}