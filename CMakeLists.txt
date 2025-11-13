cmake_minimum_required(VERSION 3.31.6)
project("libbfx" C)
include(CTest)

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Version
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLIBBFX_VERSION='\"${GIT_COMMIT_HASH}\"'")

function(Enable_Tests)
  set(TEST ON CACHE INTERNAL "")
endfunction()

function(Build_Binary)
  add_subdirectory(bfx)
  include_directories(bfx)
endfunction()

function(Build_Library)
  add_subdirectory(libbfx)
  include_directories(libbfx)
endfunction()

function(Build_Tests)
  add_subdirectory(external)
  add_subdirectory(test)
endfunction()

if(TARGET_GROUP STREQUAL test)
  Enable_Tests()
  Build_Library()
  Build_Tests()
else()
  Build_Library()
  Build_Binary()
endif()
