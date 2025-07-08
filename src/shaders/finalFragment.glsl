#version 420 core

out vec3 color;

uniform sampler2D u_Original;
uniform sampler2D u_Bloom0;
uniform sampler2D u_Bloom1;
uniform sampler2D u_Bloom2;
uniform sampler2D u_Bloom3;
// uniform sampler2D u_Bloom4;
uniform vec2 u_Resolution;

vec3 gammaCorrection (vec3 colour, float gamma) {
  return pow(colour, vec3(1. / gamma));
}

void main() {
    vec2 uv = gl_FragCoord.xy/u_Resolution;

    vec3 rgb = texelFetch(u_Original, ivec2(gl_FragCoord.xy), 0).rgb;
    rgb = gammaCorrection(rgb, 1.61803);
    rgb += texelFetch(u_Bloom0, ivec2(gl_FragCoord.xy), 0).rgb;
    rgb += texelFetch(u_Bloom1, ivec2(gl_FragCoord.xy), 0).rgb;
    rgb += texelFetch(u_Bloom2, ivec2(gl_FragCoord.xy), 0).rgb;
    rgb += texelFetch(u_Bloom3, ivec2(gl_FragCoord.xy), 0).rgb;
    // rgb += texelFetch(u_Bloom4, ivec2(gl_FragCoord.xy), 0).rgb;

    color = rgb;
    return;
}