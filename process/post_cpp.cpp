#include <iostream>
#include <csignal>
#include <unistd.h>
#include <cerrno>
#include <cstring>

int main() {
    pid_t pid;

    std::cout << "Enter the target process ID: ";
    std::cin >> pid;

    if (std::cin.fail()) {
        std::cerr << "Error: please enter a valid integer PID" << std::endl;
        return 1;
    }

    if (kill(pid, SIGUSR1) == -1) {
        std::cerr << "Error sending signal: " << std::strerror(errno) << std::endl;
    } else {
        std::cout << "Sent SIGUSR1 to process " << pid << std::endl;
    }

    return 0;
}