// socket_server.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

#define SOCKET_PATH "/tmp/my_unix_socket"
#define BUFFER_SIZE 128

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];

    // Create socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Remove any previous socket file
    unlink(SOCKET_PATH);

    // Set address
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
     {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening at %s\n", SOCKET_PATH);

    // Accept connection
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Receive message
    int bytes = read(client_fd, buffer, BUFFER_SIZE);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Server received: %s\n", buffer);
    }

    // Cleanup
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    return 0;
}
