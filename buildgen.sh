#! /bin/sh

# !IMPORTANT!: execute this file from the project root directory

rm -rf build 
mkdir build && cd build
cmake -G "Unix Makefiles" ../src
