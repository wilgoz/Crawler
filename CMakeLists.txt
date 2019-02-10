cmake_minimum_required(VERSION 3.10)
project(crawler)

include_directories(inc)

file(GLOB CORE "core/*.c")

add_executable(crawler main.c ${CORE})
target_compile_options("crawler" PRIVATE -Wall -Wextra -pedantic -Werror -g -O2)