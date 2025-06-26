#!/bin/bash
vertex_glsl_path="./src/shaders/Vertex.glsl"
fragment_glsl_path="./src/shaders/Fragment.glsl"
skybox_directory_path="./Textures/DaylightBox"
object_file_path="./src/objects/motorcycle-engine.off"


# Navigate to the build directory
./build/ray_tracer $vertex_glsl_path $fragment_glsl_path $skybox_directory_path


