#!/bin/bash
vertex_glsl_path="./src/shaders/Vertex.glsl"
fragment_glsl_path="./src/shaders/Fragment.glsl"
skybox_directory_path="./textures/BlackStudio"

BUILD_DIR="build"

# Create build directory if it doesn't exist
mkdir -p $BUILD_DIR

# Navigate to the build directory
cd $BUILD_DIR
# Run CMake to configure the project
cmake ..
# Build the project
make -j 16

# Navigate back to the root directory
cd ..

./build/ray_tracer $vertex_glsl_path $fragment_glsl_path $skybox_directory_path

