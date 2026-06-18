#!/bin/sh -e

# 1. Precompile the module interface
clang++ -std=c++20 --precompile -x c++-module figlet.cppm -o figlet.pcm

# 2. Compile the .pcm file into an object file
clang++ -std=c++20 -c figlet.pcm -o figlet.o

# 3. Compile and link figdemo.cpp (Added -lnotcurses-core here)
clang++ -std=c++20 -fmodule-file=figlet=figlet.pcm figdemo.cpp figlet.o -o figdemo $(pkg-config --libs notcurses++)
