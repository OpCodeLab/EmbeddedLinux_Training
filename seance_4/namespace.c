/*
 * ns_pid_demo.c
 *
 * Minimal demo of a Linux PID namespace created with clone(2).
 *
 * The child is started with CLONE_NEWPID: inside its own PID
 * namespace it becomes PID 1, even though the host still sees it
 * under a different "real" PID.
 *
 * Build:
 *   gcc -Wall -Wextra -o ns_pid_demo ns_pid_demo.c
 *
 * Run (creating a PID namespace needs root or CAP_SYS_ADMIN):
 *   sudo ./ns_pid_demo
 *
 * From another terminal, you can compare:
 *   ps -ef | grep ns_pid_demo        # real host pid of the child
 *   readlink /proc/<host_pid>/ns/pid # pid namespace id as seen from host
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <errno.h>

#define STACK_SIZE (1024 * 1024)   /* 1 MB stack for the child */

static char child_stack[STACK_SIZE];

static void print_pid_ns_id(const char *tag)
{
    /*
     * /proc/self/ns/pid is a symlink whose target encodes the
     * namespace's identity, e.g. "pid:[4026532345]". Two processes
     * in the same PID namespace show the same number here.
     */
    char target[64];
    ssize_t n = readlink("/proc/self/ns/pid", target, sizeof(target) - 1);
    if (n >= 0) {
        target[n] = '\0';
        printf("[%s] real pid=%d  pid namespace = %s\n", tag, getpid(), target);
    } else {
        printf("[%s] real pid=%d  readlink failed (%s)\n",
               tag, getpid(), strerror(errno));
    }
}

static int child_func(void *arg)
{
    (void)arg;

    /*
     * Inside the new PID namespace this process is PID 1.
     * Note: without also remounting /proc in a new mount namespace,
     * tools like `ps` still read the host's /proc, so they won't
     * reflect this new PID numbering -- only getpid() and
     * /proc/self/ns/pid do, from inside the child itself.
     */
    printf("\n[child] getpid() = %d "
           "(this is PID 1 inside the new PID namespace)\n", getpid());
    
    print_pid_ns_id("child");

    printf("[child] done, exiting.\n");
    return 0;
}

int main(void)
{
    printf("[parent] starting\n");
    print_pid_ns_id("parent");

    pid_t pid = clone(child_func, child_stack + STACK_SIZE,
                       CLONE_NEWPID | SIGCHLD, NULL);
    if (pid == -1)
     {
        perror("clone");
        fprintf(stderr,
            "Hint: creating a PID namespace usually requires root "
            "(or CAP_SYS_ADMIN). Try: sudo ./ns_pid_demo\n");
        exit(EXIT_FAILURE);
    }

    printf("[parent] created child, host-visible pid=%d\n", pid);

    if (waitpid(pid, NULL, 0) == -1) {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }

    printf("[parent] child has exited.\n");
    return 0;
}