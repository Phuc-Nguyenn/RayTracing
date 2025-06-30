#version 420 core

out vec3 color;

vec2 u_ScreenResolution;
uniform sampler2D u_Image;


void main() {
    vec2 uv = gl_FragCoord.xy / u_ScreenResolution;
    
    color = texture(u_Image, uv).rgb;
}