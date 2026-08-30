#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 8080

int main(void)
{
    fprintf(stderr, "[1] main started\n");

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {

        /* CHILD */
        fprintf(stderr, "[CHILD] PID=%d\n", getpid());

        FILE *fp = popen("ls -l", "r");

        if (fp == NULL) {
            perror("popen");
            exit(1);
        }

        fprintf(stderr, "[CHILD] popen succeeded\n");

        char buffer[256];

        while (fgets(buffer, sizeof(buffer), fp)) {
            fprintf(stderr, "[CHILD] %s", buffer);
        }

        pclose(fp);

        fprintf(stderr, "[CHILD] ls finished\n");

        exit(0);
    }

    /* PARENT */
    fprintf(stderr, "[PARENT] PID=%d\n", getpid());

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    fprintf(stderr, "[PARENT] socket created\n");

    int opt = 1;

    setsockopt(server_fd,
               SOL_SOCKET,
               SO_REUSEADDR,
               &opt,
               sizeof(opt));

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {

        perror("bind");
        close(server_fd);
        return 1;
    }

    fprintf(stderr, "[PARENT] bind successful\n");

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    fprintf(stderr,
            "[PARENT] Server listening on port %d\n",
            PORT);

    waitpid(pid, NULL, 0);

    fprintf(stderr, "[PARENT] Child finished\n");

    while (1) {

        fprintf(stderr, "[PARENT] Waiting for client...\n");

        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        fprintf(stderr, "[PARENT] Client connected\n");

        const char *response =
            "Hello from Arch Linux Docker container!\n";

        write(client_fd, response, strlen(response));

        close(client_fd);
    }

    close(server_fd);

    return 0;
}