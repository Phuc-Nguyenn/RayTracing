#!/bin/bash
BUILD_DIR="./build"

# Run CMake to generate build files
cmake -B $BUILD_DIR

cmake --build $BUILD_DIR -j 16

