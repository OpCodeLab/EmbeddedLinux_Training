cmake_minimum_required(VERSION 3.10)
project(LinkingDemo C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Pull in the two library subprojects
add_subdirectory(static_lib)
add_subdirectory(shared_lib)

# The main executable
add_executable(demo main.c)

# Link BOTH libraries into the final binary:
#  - strutils  -> STATIC  -> code gets copied into "demo" at link time
#  - mathops   -> SHARED  -> "demo" only stores a reference; mathops.so
#                            is loaded when the program runs
target_link_libraries(demo PRIVATE strutils mathops)
