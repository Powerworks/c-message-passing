cmake_minimum_required(VERSION 3.10)
project(c_message_passing C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(c-message-passing
    src/actor.c
    src/mailbox.c
    src/main.c
)

target_include_directories(c-message-passing PRIVATE src)
