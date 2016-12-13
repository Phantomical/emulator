cmake_minimum_required(VERSION 2.6)

set(SRCS driver.c emulator.c syscalls.c)
add_executable(emulator ${SRCS})
