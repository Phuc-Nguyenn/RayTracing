#!/bin/bash
vertex_glsl_path="./src/shaders/Vertex.glsl"
fragment_glsl_path="./src/shaders/Fragment.glsl"
skybox_directory_path="./Textures/DaylightBox"
object_paths="
./Objects/motorcycle-engine.off"

# ./Objects/sponza-palace.off
# ./Objects/dragon.off
# ./Objects/sharpsphere.off
# ./Objects/livingroom.off
# ./Objects/teapot1.txt
# ./Objects/teapot2.txt

# Navigate to the build directory
./build/ray_tracer $vertex_glsl_path $fragment_glsl_path $skybox_directory_path $object_paths





