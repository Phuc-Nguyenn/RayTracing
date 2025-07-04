#!/bin/bash
vertex_glsl_path="./src/shaders/Vertex.glsl"
fragment_glsl_path="./src/shaders/Fragment.glsl"
skybox_directory_path="./Textures/DaylightBox"
object_paths="
../Objects/sponza.off
../Objects/dragon.off
../Objects/glassDragon.off
../Objects/cube_light_R.txt
../Objects/cube_light_G.txt
../Objects/cube_light_B.txt
../Objects/cube_light_W.txt
../Objects/teapot3.txt
../Objects/bunny70k.off
../Objects/monkey.off
"

# ../Objects/sponza.off
# ../Objects/dragon.off
# ../Objects/cube_light_R.txt
# ../Objects/cube_light_G.txt
# ../Objects/cube_light_B.txt
# ../Objects/cube_light_W.txt
# ../Objects/bunny70k.off

# ../Objects/sponza-palace.off
# ../Objects/dragon.off
# ../Objects/sharpsphere.off
# ../Objects/livingroom.off
# ../Objects/teapot1.txt
# ../Objects/teapot2.txt

# ../Objects/sharpsphere.off
# ../Objects/livingroom.off
# ../Objects/teapot1.txt
# ../Objects/teapot2.txt

# Navigate to the build directory
./build/ray_tracer $vertex_glsl_path $fragment_glsl_path $skybox_directory_path $object_paths
