#version 420 core

out vec3 color;

uniform sampler2D u_Image;

void main() {
    vec2 uv = gl_FragCoord.xy / screenResolution;
    
    color = texture(u_Image, uv).rgb;
}