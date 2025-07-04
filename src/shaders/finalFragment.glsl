#version 420 core

out vec3 color;

uniform sampler2D u_Original;
uniform sampler2D u_Bloom0;
uniform sampler2D u_Bloom1;
uniform sampler2D u_Bloom2;
uniform sampler2D u_Bloom3;
// uniform sampler2D u_Bloom4;
uniform vec2 u_Resolution;

#define INV_SQRT_OF_2PI 0.39894228040143267793994605993439  // 1.0/SQRT_OF_2PI
#define INV_PI 0.31830988618379067153776752674503

vec3 gammaCorrection (vec3 colour, float gamma) {
  return pow(colour, vec3(1. / gamma));
}

vec4 smartDeNoise(sampler2D tex, vec2 uv, float sigma, float kSigma, float threshold)
{
    float radius = round(kSigma*sigma);
    float radQ = radius * radius;

    float invSigmaQx2 = .5 / (sigma * sigma);      // 1.0 / (sigma^2 * 2.0)
    float invSigmaQx2PI = INV_PI * invSigmaQx2;    // 1/(2 * PI * sigma^2)

    float invThresholdSqx2 = .5 / (threshold * threshold);     // 1.0 / (sigma^2 * 2.0)
    float invThresholdSqrt2PI = INV_SQRT_OF_2PI / threshold;   // 1.0 / (sqrt(2*PI) * sigma^2)

    vec4 centrPx = texture(tex,uv); 

    float zBuff = 0.0;
    vec4 aBuff = vec4(0.0);
    vec2 size = vec2(textureSize(tex, 0));

    vec2 d;
    for (d.x=-radius; d.x <= radius; d.x++) {
        float pt = sqrt(radQ-d.x*d.x);       // pt = yRadius: have circular trend
        for (d.y=-pt; d.y <= pt; d.y++) {
            float blurFactor = exp( -dot(d , d) * invSigmaQx2 ) * invSigmaQx2PI;

            vec4 walkPx =  texture(tex,uv+d/size);
            vec4 dC = walkPx-centrPx;
            float deltaFactor = exp( -dot(dC, dC) * invThresholdSqx2) * invThresholdSqrt2PI * blurFactor;

            zBuff += deltaFactor;
            aBuff += deltaFactor*walkPx;
        }
    }
    return aBuff/zBuff;
}

void main() {
    vec2 uv = gl_FragCoord.xy/u_Resolution;

    vec3 rgb = smartDeNoise(u_Original, uv, 0.7, 0.8, 0.05).rgb;
    rgb = gammaCorrection(rgb, 1.61803);
    
    // vec3 rgb = texelFetch(u_Original, ivec2(gl_FragCoord.xy), 0).rgb;
    // rgb = gammaCorrection(rgb, 1.61803);
    rgb += texelFetch(u_Bloom0, ivec2(gl_FragCoord.xy), 0).rgb;
    rgb += texelFetch(u_Bloom1, ivec2(gl_FragCoord.xy), 0).rgb;
    rgb += texelFetch(u_Bloom2, ivec2(gl_FragCoord.xy), 0).rgb;
    rgb += texelFetch(u_Bloom3, ivec2(gl_FragCoord.xy), 0).rgb;
    // rgb += texelFetch(u_Bloom4, ivec2(gl_FragCoord.xy), 0).rgb;

    color = rgb;
    return;
}