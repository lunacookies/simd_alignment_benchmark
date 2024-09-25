#!/bin/sh
clang-format -i main.c
clang -o simd_alignment_benchmark -Os -g3 -fwrapv -fno-strict-aliasing -fmodules main.c
