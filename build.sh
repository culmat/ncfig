#!/bin/sh -e

# 1. Precompile the module interface
clang++ -std=c++20 --precompile -x c++-module src/figlet.cppm -o src/figlet.pcm

# 2. Compile the .pcm file into an object file
clang++ -std=c++20 -c src/figlet.pcm -o src/figlet.o

# 3. Compile and link figdemo.cpp (Added -lnotcurses-core here)
clang++ -std=c++20 -fmodule-file=figlet=src/figlet.pcm src/figdemo.cpp src/figlet.o -o figdemo $(pkg-config --libs notcurses++)
