#version 420 core

out vec3 color;

uniform sampler2D u_Image;
uniform vec2 u_ImageResolution;


vec4 colorCorrection(vec3 col) {
    float level = max(col.x, max(col.y, col.z));
    if (level > 1.0) {
        col = col/level;
        col.r = pow(col.r,1./level);
        col.g = pow(col.g,1./level);
        col.b = pow(col.b,1./level);
    }
    return vec4(col,level);
}

vec3 bloom(sampler2D image, vec2 pixel, vec2 iResolution, float intensity, float threshold, int steps) {
    vec3 col = vec3(0);
    vec4 coldat = colorCorrection(texelFetch(image,ivec2(pixel),0).rgb);
    
    int i=1;
    steps = max(steps,1);
    int radius = 4;
    float screenrad = 0.1;
    float aspect = iResolution.x/iResolution.y;
    for (int x=-radius; x<=radius; x+=steps) {
        for (int y=-radius; y<=radius; y+=steps) {
            if (length(vec2(x,y))<=float(radius)) {
            vec2 coord = vec2(x,y);
            vec4 newcol = colorCorrection(texelFetch(image,ivec2(pixel+coord),0).rgb);
            if (newcol.w >= threshold) {
                col += clamp(newcol.rgb*newcol.w*intensity/(length(coord/radius)), 0.0, 2.0);
            }
            i++;
            }
        }
    }
    return col/max(i, 1);
}

void main() {
    color = bloom(u_Image, gl_FragCoord.xy, u_ImageResolution, 0.5, 1.01, 1);
}