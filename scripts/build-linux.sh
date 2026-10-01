#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
project_dir=$(pwd)
cmake_bin=$(command -v cmake || true)
ninja_bin=$(command -v ninja || true)
if [ -z "$cmake_bin" ]; then
    cmake_bin="$project_dir/.tools/linux/cmake-3.31.6-linux-x86_64/bin/cmake"
fi
if [ -z "$ninja_bin" ]; then
    ninja_bin="$project_dir/.tools/linux/ninja"
fi
if [ ! -x "$cmake_bin" ] || [ ! -x "$ninja_bin" ]; then
    echo 'Install CMake >= 3.24 and Ninja (Ubuntu: sudo apt-get install cmake ninja-build).' >&2
    exit 2
fi
"$cmake_bin" --preset linux-debug "-DCMAKE_MAKE_PROGRAM=$ninja_bin" "$@"
"$cmake_bin" --build --preset linux-debug
"$(dirname "$cmake_bin")/ctest" --preset linux-debug
