#!/bin/bash
BUILD_DIR="build"

# Create build directory if it doesn't exist
mkdir -p $BUILD_DIR

# Run CMake to generate build files
cmake -S . -B $BUILD_DIR

# Navigate to the build directory
cd $BUILD_DIR
# Build the project
make -j 16
# Navigate back to the root directory
cd ..

