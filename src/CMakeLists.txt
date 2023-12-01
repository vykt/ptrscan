cmake_minimum_required(VERSION 3.13)

#define build tools
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

#define project
project(ptrscan VERSION 0.0.1 DESCRIPTION "ptrscan for a linux environment" LANGUAGES C CXX)

#set sources
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/target)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ptrscan)

#define targets
add_executable(example_proc ${SOURCES_TARGET} ${HEADERS_TARGET})
add_executable(scan ${SOURCES_PTRSCAN} ${HEADERS_PTRSCAN})
add_executable(scan_debug ${SOURCES_PTRSCAN} ${HEADERS_PTRSCAN})

#example process options
target_compile_features(example_proc PUBLIC cxx_std_14)
target_compile_options(example_proc PUBLIC -ggdb -O0 -Wall)

#ptrscan options
target_compile_features(scan PUBLIC cxx_std_14)
target_compile_options(scan PUBLIC -ggdb -O0 -Wall)
target_link_options(scan PUBLIC -lpwu -lpthread)

#ptrscan debug options
target_compile_features(scan_debug PUBLIC cxx_std_14)
target_compile_options(scan_debug PUBLIC -D DEBUG -ggdb -O0 -Wall)
target_link_options(scan_debug PUBLIC -lpwu -lpthread)
