#!/bin/bash

OPTIMIZE='-O0 -fno-strict-aliasing'
WARNS='-Wall -Wextra -Werror'

set -e
mkdir -p bin
rm -f bin/*
gcc -std=gnu99 -ggdb3 $OPTIMIZE $WARNS -Isrc src/main.c -o bin/buildbuddy

