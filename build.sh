#!/bin/bash

OPTIMIZE='-O2 -fno-strict-aliasing'
WARNS='-Wall -Wextra -Werror'

set -e
mkdir -p bin
rm -f bin/*
gcc -std=gnu99 $OPTIMIZE $WARNS -Isrc src/main.c -o bin/buildbuddy

