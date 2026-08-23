// shm_reader.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>    // For O_* constants
#include <sys/mman.h> // For shm_open, mmap
#include <unistd.h>
#include <string.h>

#define SHM_NAME "/my_shm"
#define SHM_SIZE 1024

int main() {
    // Open existing shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    // Map it into address space
    void *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Read and print message
    printf("Reader read: %s\n", (char *)shm_ptr);

    // Cleanup
    munmap(shm_ptr, SHM_SIZE);
    close(shm_fd);

    return 0;
}
