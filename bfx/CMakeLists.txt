cmake_minimum_required(VERSION 3.31.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I../libbfx/src")
add_executable(bfx-bin src/bfx.c)
target_link_libraries(bfx-bin bfx)
