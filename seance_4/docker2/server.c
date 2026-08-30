#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define PORT 8080
#define BACKLOG 5

int main(void)
{
    fprintf(stderr, "[MAIN] PID=%d started\n", getpid());

    /*
     * Create child process
     */
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    /*
     * =========================
     * CHILD PROCESS
     * =========================
     */
    if (pid == 0) {

        fprintf(stderr,
                "[CHILD] PID=%d, PPID=%d\n",
                getpid(),
                getppid());

        FILE *fp = popen("ls -l", "r");

        if (fp == NULL) {
            perror("[CHILD] popen");
            exit(EXIT_FAILURE);
        }

        fprintf(stderr, "[CHILD] popen() succeeded\n");

        char buffer[256];

        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            fprintf(stderr, "[CHILD] %s", buffer);
        }

        int status = pclose(fp);

        if (status == -1) {
            perror("[CHILD] pclose");
            exit(EXIT_FAILURE);
        }

        fprintf(stderr,
                "[CHILD] ls finished, status=%d\n",
                status);

        exit(EXIT_SUCCESS);
    }

    /*
     * =========================
     * PARENT PROCESS
     * =========================
     */

    fprintf(stderr,
            "[PARENT] PID=%d, CHILD PID=%d\n",
            getpid(),
            pid);

    /*
     * Create TCP socket
     */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("[PARENT] socket");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "[PARENT] socket created: fd=%d\n", server_fd);

    /*
     * Allow address reuse
     */
    int opt = 1;

    if (setsockopt(server_fd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &opt,
                   sizeof(opt)) < 0) {

        perror("[PARENT] setsockopt");
        close(server_fd);
        return EXIT_FAILURE;
    }

    /*
     * Configure server address
     */
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    /*
     * Bind socket
     */
    if (bind(server_fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {

        perror("[PARENT] bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    fprintf(stderr,
            "[PARENT] bind successful: port=%d\n",
            PORT);

    /*
     * Start listening
     */
    if (listen(server_fd, BACKLOG) < 0) {

        perror("[PARENT] listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    fprintf(stderr,
            "[PARENT] Server listening on 0.0.0.0:%d\n",
            PORT);

    /*
     * Do NOT block the server with waitpid().
     *
     * WNOHANG checks whether the child has finished
     * without blocking the TCP server.
     */

    while (1) {

        /*
         * Check child status
         */
        int child_status;
        pid_t result = waitpid(pid, &child_status, WNOHANG);

        if (result == pid) {

            if (WIFEXITED(child_status)) {
                fprintf(stderr,
                        "[PARENT] Child exited with status=%d\n",
                        WEXITSTATUS(child_status));
            }
            else if (WIFSIGNALED(child_status)) {
                fprintf(stderr,
                        "[PARENT] Child killed by signal=%d\n",
                        WTERMSIG(child_status));
            }

            /*
             * Set pid to -1 so we don't wait for it again.
             */
            pid = -1;
        }
        else if (result < 0 && errno != ECHILD) {
            perror("[PARENT] waitpid");
        }

        fprintf(stderr,
                "[PARENT] Waiting for client...\n");

        /*
         * Accept TCP connection
         */
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd < 0) {

            if (errno == EINTR)
                continue;

            perror("[PARENT] accept");
            continue;
        }

        fprintf(stderr,
                "[PARENT] Client connected: fd=%d\n",
                client_fd);

        /*
         * Send response
         */
        const char response[] =
            "Hello from Arch Linux Docker container!\n";

        ssize_t ret = send(client_fd,
                           response,
                           strlen(response),
                           0);

        if (ret < 0) {
            perror("[PARENT] send");
        }
        else {
            fprintf(stderr,
                    "[PARENT] Sent %zd bytes\n",
                    ret);
        }

        /*
         * Close client connection
         */
        close(client_fd);

        fprintf(stderr,
                "[PARENT] Client disconnected\n");
    }

    close(server_fd);

    return EXIT_SUCCESS;
}
