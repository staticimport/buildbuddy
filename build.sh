#!/bin/bash

OPTIMIZE='-O2 -fno-strict-aliasing'
WARNS='-Wall -Wextra -Werror'

set -e
mkdir -p bin
rm -f bin/*
gcc -std=gnu99 -D_GNU_SOURCE $OPTIMIZE $WARNS -ggdb3 -Isrc src/main.c -o bin/buildbuddy

