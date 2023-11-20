cmake_minimum_required(VERSION 3.12)

#define build tools
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

#define project
project(ptrscan VERSION 0.0.1 DESCRIPTION "ptrscan for a linux environment" LANGUAGES C CXX)

#set sources
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/target)

message(STATUS "sources: ${SOURCES_TARGET} \nheaders: ${HEADERS_TARGET}")

#define targets
add_executable(example_proc ${SOURCES_TARGET} ${HEADERS_TARGET})

target_compile_features(example_proc PUBLIC cxx_std_14)
target_compile_options(example_proc PUBLIC -ggdb -O0 -Wall)
