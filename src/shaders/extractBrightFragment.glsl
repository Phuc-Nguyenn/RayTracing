#version 420 core

out vec3 color;

uniform sampler2D u_Image;

#define THRESHOLD 1.5

void main() {
    vec3 rgb = texelFetch(u_Image, ivec2(gl_FragCoord.xy), 0).rgb;
    if(length(rgb) >= THRESHOLD) {
        color = rgb;
    } else {
        color = vec3(0.0);
    }
}