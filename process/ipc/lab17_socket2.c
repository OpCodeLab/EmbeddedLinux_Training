// socket_client.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

#define SOCKET_PATH "/tmp/my_unix_socket"

int main() {
    int client_fd;
    struct sockaddr_un addr;
    const char *message = "Hello from client!";

    // Create socket
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set address
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Connect to server
    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Send message
    write(client_fd, message, strlen(message));
    printf("Client sent: %s\n", message);

    // Cleanup
    close(client_fd);

    return 0;
}
