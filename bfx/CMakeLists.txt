cmake_minimum_required(VERSION 3.31.6)

include_directories(../libbfx/src)
add_executable(bfx-bin src/main.c)
target_link_libraries(bfx-bin PRIVATE bfx)
