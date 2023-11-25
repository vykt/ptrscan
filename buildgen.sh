#! /bin/sh

# !!! IMPORTANT !!!
# execute this file from the build directory:
#
# mkdir ./build && cd ./build && ../buildgen.sh

rm -rf build 
mkdir build && cd build
cmake -G "Unix Makefiles" ../src
