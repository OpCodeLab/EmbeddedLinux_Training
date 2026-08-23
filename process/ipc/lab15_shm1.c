// shm_writer.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>    // For O_* constants
#include <sys/mman.h> // For shm_open, mmap
#include <unistd.h>   // For ftruncate
#include <string.h>

#define SHM_NAME "/my_shm"
#define SHM_SIZE 1024

int main() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    // Set size of shared memory
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(1);
    }

    // Map shared memory into address space
    void *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Write to shared memory
    const char *message = "Hello from writer!";
    memcpy(shm_ptr, message, strlen(message) + 1);

    printf("Writer wrote: %s\n", (char *)shm_ptr);

    // Keep it alive for reading
    sleep(5);

    // Cleanup
    munmap(shm_ptr, SHM_SIZE);
    close(shm_fd);
    shm_unlink(SHM_NAME); // Remove the shared memory object

    return 0;
}
