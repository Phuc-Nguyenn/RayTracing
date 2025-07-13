#version 430 core

out vec3 color;

#define RAY_COUNT 1
#define FOG_DENSITY 0.00
#define FOG_HEIGHT 32.0
#define AIR_REFRACT 1.0003
#define MAX_STACK_SIZE 64
#define INF 1.0/0.0
#define MAX_HITTABLE_COUNT 4
#define MAX_MATERIALS_COUNT 16
#define SPHERE 0

struct Material {
    vec3 colour;
    vec3 specularColour;
    float roughness;
    float metallic;
    float transparency;
    float refractionIndex;
    bool isLight;
};

struct Hittable { // are things like spheres and triangles
    uint type;
    vec3 position;
    vec3 direction;
    float scale;   
    Material material;
};

struct Sphere {
    vec3 position;
    float scale;
};

struct Triangle {
    vec3 position;
    vec3 position2;
    vec3 position3;
};

layout(std430, binding = 0) buffer B_Triangles
{
    Triangle trianglesBuffer[];
};

struct BoundingBox {
    vec3 maxi;
    vec3 mini;
    int triangleCount;
    int triangleStartIndex;
    int rightChildIndex;
};

struct Camera {
    vec3 position;
    vec3 facing;
    float fov;
    float viewportWidth;
    float viewportDistance;
};

struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 invDirection;
    float magnitude;
};

uniform vec2 screenResolution;
uniform uint u_BounceLimit;
uniform Camera u_Camera;
uniform Hittable u_Hittable[MAX_HITTABLE_COUNT];
uniform uint u_HittablesCount;

// for progressive rendering
uniform uint u_FrameIndex;
uniform vec3 u_RandSeed;
uniform sampler2D u_Accumulation;
uniform sampler2D u_RgbNoise;
uniform vec2 u_RgbNoiseResolution;

uniform isamplerBuffer  u_MaterialsIndex; // int

uniform samplerBuffer u_BoundingBoxes; // r32f
uniform uint u_BoundingBoxesCount;

uniform Material u_Materials[MAX_MATERIALS_COUNT];
uniform uint u_MaterialsCount;

// skybox
uniform samplerCube u_Skybox;

uint Hash(uint x) {
    x ^= x >> 17;
    x *= 0xed5ad4bbU;
    x ^= x >> 11;
    x *= 0xac4c1b51U;
    x ^= x >> 15;
    x *= 0x31848babU;
    x ^= x >> 14;
    return x;
}

uint Hashf(float x) {
    return Hash(floatBitsToUint(x));
}

struct HitRecord {
    vec3 hitPoint;
    vec3 normal;
    float t;
    bool frontFace;
    bool hitAnything;
    int index;
    Material material;
};

void SetFaceNormal(inout HitRecord rec, Ray r, vec3 outwardNormal) {
    rec.frontFace = dot(r.direction, outwardNormal) < 0;
    if (rec.frontFace) {
        rec.normal = outwardNormal;
        return;
    }
    rec.normal = -outwardNormal;
};

vec3 RayAt(Ray ray, float t) {
    vec3 pos = ray.origin + ray.magnitude*t*ray.direction;
    return pos;
};

Ray MakeRay(vec3 origin, vec3 direction, float magnitude) {
    Ray new;
    new.origin = origin;
    new.direction = direction;
    new.invDirection = 1.0 / direction;
    new.magnitude = magnitude;
    return new;
};

vec3 directionToViewport(vec2 randOffset) {
    float viewportHeight = u_Camera.viewportWidth * (screenResolution.y / screenResolution.x);

    vec3 viewportUdir = normalize(cross(u_Camera.facing, vec3(0, 0, 1)));
    vec3 viewportVdir = normalize(cross(viewportUdir, u_Camera.facing));

    vec3 initialRay = u_Camera.position 
        + u_Camera.viewportDistance * u_Camera.facing 
        + u_Camera.viewportWidth * (-0.5 + (gl_FragCoord.x - 0.5 + randOffset.x) / screenResolution.x) * viewportUdir 
        + viewportHeight * (-0.5 + (gl_FragCoord.y - 0.5 + randOffset.y) / screenResolution.y) * viewportVdir;

    return normalize(initialRay - u_Camera.position);
}

vec3 RotateAroundAxis(vec3 v, vec3 k, float a) {
    float cosA = cos(a);
    float sinA = sin(a);
    vec3 result = v * cosA + cross(k,v) * sinA + k * dot(k,v) * (1 - cosA);
    return result;
}

bool hitSphere(Sphere sphere, Ray r, inout HitRecord hitRecord) {
    vec3 center = sphere.position;
    float radius = sphere.scale;
    vec3 oc = center - r.origin;
    float a = dot(r.direction, r.direction);
    float b = -2.0 * dot(r.direction, oc);
    float c = dot(oc, oc) - radius*radius;
    float discriminant = b*b - 4*a*c;

    if (discriminant < 0)
        return false;
    float root = (-b - sqrt(discriminant) ) / (2.0*a);
    if (root < 0.0) {
        root = (-b + sqrt(discriminant) ) / (2.0*a);
        if(root < 0.0)
            return false;
    }

    hitRecord.t = root;
    hitRecord.hitPoint = RayAt(r, hitRecord.t);
    vec3 outwardNormal = (hitRecord.hitPoint - center) / radius;
    SetFaceNormal(hitRecord, r, outwardNormal);
    hitRecord.index = -1;
    return true;
};

Triangle getTriangle(int index) {
    return trianglesBuffer[index];
}

BoundingBox getBoundingBox(int index) {
    BoundingBox box;
    int bufferIndex = index * 2;
    vec4 first = texelFetch(u_BoundingBoxes, bufferIndex + 0);
    vec4 second = texelFetch(u_BoundingBoxes, bufferIndex + 1);
    box.maxi = first.xyz;
    box.mini = vec3(first.w, second.xy);
    box.triangleCount = int(second.z);
    second.z >= 1 ? box.triangleStartIndex = int(second.w) : box.rightChildIndex = int(second.w);
    return box;
}

bool hitTriangle(inout Triangle triangle, Ray r, inout HitRecord hitrecord) {
    vec3 v0 = triangle.position;
    vec3 v1 = triangle.position2;
    vec3 v2 = triangle.position3;

    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;
    vec3 h = cross(r.direction, e2);
    float det = dot(e1, h);

    if (abs(det) < 1e-8)
        return false; // Ray is parallel to triangle

    float invDet = 1.0 / det;
    vec3 s = r.origin - v0;
    float u = invDet * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;

    vec3 q = cross(s, e1);
    float v = invDet * dot(r.direction, q);
    if (v < 0.0 || u + v > 1.0)
        return false;

    float t = invDet * dot(e2, q);
    if (t < 0.0001)
        return false;

    hitrecord.t = t;
    hitrecord.hitPoint = RayAt(r, t);
    vec3 normal = normalize(cross(e1, e2));
    SetFaceNormal(hitrecord, r, normal);
    return true;
}

bool hitBoundingBox(const Ray ray, const BoundingBox aabb, out HitRecord hitRecord) {
    vec3 t0s = (aabb.mini - ray.origin) * ray.invDirection;
    vec3 t1s = (aabb.maxi - ray.origin) * ray.invDirection;

    vec3 tsmaller = min(t0s, t1s);
    vec3 tbigger = max(t0s, t1s);

    float tmin = max(max(tsmaller.x, tsmaller.y), tsmaller.z);
    float tmax = min(min(tbigger.x, tbigger.y), tbigger.z);

    hitRecord.hitAnything = bool(tmax >= max(tmin, 0.0));
    hitRecord.t = tmin;

    // Find which axis was hit for the normal
    // vec3 normal = vec3(0.0);
    // if (tmin == tsmaller.x) normal = vec3(-sign(ray.direction.x), 0.0, 0.0);
    // else if (tmin == tsmaller.y) normal = vec3(0.0, -sign(ray.direction.y), 0.0);
    // else if (tmin == tsmaller.z) normal = vec3(0.0, 0.0, -sign(ray.direction.z));
    // hitRecord.normal = normal;

    return hitRecord.hitAnything;
}

bool HitHittableList(Ray ray, inout HitRecord hitRecord) {
    hitRecord.t = INF;
    hitRecord.hitAnything = false;
    hitRecord.index = -1;

    int stack[MAX_STACK_SIZE];
    int stackptr = 0;
    if(u_BoundingBoxesCount > 0) 
        stack[stackptr++] = 0;
    int leafhitNum = 0;
    int iterationsCount = 0;
    while(stackptr > 0) {
        int indexBB = stack[--stackptr];
        BoundingBox aabb = getBoundingBox(indexBB);
        HitRecord hitRecordTmp;
        iterationsCount += 1;
        if(!hitBoundingBox(ray, aabb, hitRecordTmp) || hitRecordTmp.t >= hitRecord.t) { 
            continue;
        }
        if(aabb.triangleCount > 0 && aabb.triangleStartIndex >= 0){ // is leaf
            for(int i=aabb.triangleStartIndex; i<aabb.triangleStartIndex + aabb.triangleCount; ++i) {
                Triangle triangle = getTriangle(i);
                if(hitTriangle(triangle, ray, hitRecordTmp)) {
                    if(hitRecord.t > hitRecordTmp.t) {
                        hitRecord = hitRecordTmp;
                        hitRecord.index = i;
                        hitRecord.material = u_Materials[texelFetch(u_MaterialsIndex, i).x];
                        hitRecord.hitAnything = true;  
                    }
                }
            }
        } else{
            // push the closer child on first
            BoundingBox leftBox = getBoundingBox(indexBB + 1);
            BoundingBox rightBox = getBoundingBox(aabb.rightChildIndex);
            HitRecord hitLeftRecord;
            HitRecord hitRightRecord;
            hitBoundingBox(ray, leftBox, hitLeftRecord);
            hitBoundingBox(ray, rightBox, hitRightRecord);
            if(hitLeftRecord.t < hitRightRecord.t) {
                if(hitRightRecord.hitAnything && hitRightRecord.t < hitRecord.t) stack[stackptr++] = aabb.rightChildIndex;
                if(hitLeftRecord.hitAnything && hitLeftRecord.t < hitRecord.t) stack[stackptr++] = indexBB + 1;
            } else {
                if(hitLeftRecord.hitAnything && hitLeftRecord.t < hitRecord.t) stack[stackptr++] = indexBB + 1;
                if(hitRightRecord.hitAnything && hitRightRecord.t < hitRecord.t) stack[stackptr++] = aabb.rightChildIndex;
            }
        }
    }
    // Color is white if iterationsCount is below threshold, otherwise gets more red as iterationsCount increases
    if (u_BounceLimit == 0) {
        int threshold1 = 64;
        int threshold2 = 128;
        if (iterationsCount < threshold1) {
            hitRecord.material.colour = (vec3(1.0)/threshold1)*iterationsCount;
        } else if(iterationsCount < threshold2) {
            hitRecord.material.colour =  vec3(1.0) - (vec3(0.0,1.0,1.0)/threshold1)*(iterationsCount - threshold1);
        } else {
            hitRecord.material.colour = vec3(1.0, 0.0, 0) - (vec3(1.0, 0, 0)/(threshold2-threshold1))*(iterationsCount - threshold2);
        }
    }
    return hitRecord.hitAnything;
};

vec3 TransparentScatter(inout HitRecord hitRecord, inout Material material, inout Ray ray) {
    float ri = hitRecord.frontFace ? AIR_REFRACT/material.refractionIndex : material.refractionIndex/AIR_REFRACT;
    float cos_theta = min(dot(-ray.direction, hitRecord.normal), 1.0);
    float sin_theta = sqrt(1.0 - cos_theta*cos_theta);
    bool canRefract = ri * sin_theta <= 1.0;
    if (canRefract)
        return refract(ray.direction, hitRecord.normal, ri);
    return reflect(ray.direction, hitRecord.normal);
}

bool VolumetricScatter(inout HitRecord hitRecord, inout Ray ray, inout vec3 colour, vec2 seed, const float maxFogTravel) {
    // find out if ray will across the fod boundary
    if(FOG_DENSITY == 0) {
        return false;
    }
    float scatterProbability = 0.0;
    bool scattered = false;

    Ray fogScope = MakeRay(ray.origin, ray.direction, 0.0);
    float distanceToFog = (FOG_HEIGHT - ray.origin.z) * ray.invDirection.z ;

    if (ray.direction.z >= 0.0 && ray.origin.z < FOG_HEIGHT) {
        fogScope.magnitude = min(distanceToFog, hitRecord.t);
    } else if (ray.direction.z < 0.0 && ray.origin.z > FOG_HEIGHT && distanceToFog < hitRecord.t) {
        fogScope.magnitude = hitRecord.t - distanceToFog;
        fogScope.origin = ray.origin + ray.direction * distanceToFog;
    } else if (ray.direction.z <= 0.0 && ray.origin.z <= FOG_HEIGHT) { // ray is inside the fog
        fogScope.magnitude = hitRecord.t;
    }

    bool hitsLight = false;
    if(hitRecord.hitAnything && hitRecord.material.isLight) 
        hitsLight = true;

    fogScope.magnitude = min(fogScope.magnitude, maxFogTravel);
    if (fogScope.magnitude > 0.0 && fogScope.magnitude <= hitRecord.t) {
        // probability of scattering is proportional to the distance traveled in fog
        scatterProbability = FOG_DENSITY * fogScope.magnitude;
        scattered = texture(u_RgbNoise, seed+vec2(0.02)).x < scatterProbability;
        if(scattered) {
            vec3 newDirection;
            if(hitsLight) 
                newDirection = normalize((1/hitRecord.t)*ray.direction + texture(u_RgbNoise, seed+vec2(-0.0123, 0.03443)).rgb * 2 - 1);
            else
                newDirection = normalize(texture(u_RgbNoise, seed-vec2(0.054)).rgb * 2 - 1);
            ray = MakeRay(fogScope.origin + ray.direction * fogScope.magnitude * texture(u_RgbNoise, seed-vec2(0.0133, 0.0552)).x, newDirection, 1);
            colour *= (1 - FOG_DENSITY);
        }
    }
    return scattered;
}

vec3 RayColour(Ray ray, vec3 lastColour, out vec3 albedo, out vec3 normal, int iRayCountSeed) {
    HitRecord hitRecord;
    vec3 rayColour = vec3(1.0);
    const float maxFogTravel = 1.0/FOG_DENSITY;
    for(int i=0; i<=u_BounceLimit; ++i) {
        bool hitAnything = HitHittableList(ray, hitRecord);
        if (u_BounceLimit == 0)
            return vec3(hitRecord.material.colour);
        if (u_BounceLimit == 1)
            return vec3(abs(hitRecord.normal));

        vec2 seed = mod(vec2(
            Hashf(ray.origin.x + ray.direction.y) + Hash(i) + Hash(iRayCountSeed) + Hash(u_FrameIndex) + Hashf(u_RandSeed.x * u_BounceLimit),
            Hashf(ray.origin.y + ray.direction.z) + Hash(i+69) + Hash(iRayCountSeed) + Hashf(u_RandSeed.y * u_BounceLimit)
        ), vec2(u_RgbNoiseResolution.x, u_RgbNoiseResolution.y))/vec2(u_RgbNoiseResolution.x, u_RgbNoiseResolution.y);

        if (VolumetricScatter(hitRecord, ray, rayColour, seed-vec2(0.31), maxFogTravel)) {
           continue;
        }

        // do the albedo
        if(i == 0) {
            if(hitAnything) {
                albedo = hitRecord.material.colour;
                normal = hitRecord.normal;
            } else {
                albedo = texture(u_Skybox, RotateAroundAxis(ray.direction, vec3(1,0,0), -3.1415/2)).rgb; 
            }
        }

        if(!hitAnything) {
            vec3 skybox_texture = texture(u_Skybox, RotateAroundAxis(ray.direction, vec3(1,0,0), -3.1415/2)).rgb;
            return rayColour*skybox_texture;
        } 
        Material material = hitRecord.material;
        if (material.isLight) {
            float luminance = material.transparency;
            return rayColour * material.colour * luminance;
        }
        vec3 nextDirection;
        float transparencyRng = texture(u_RgbNoise, seed+vec2(0.01534, 0.183)).x;
        if(transparencyRng < material.transparency) {
            nextDirection = TransparentScatter(hitRecord, material, ray);
            if(transparencyRng < 0.5)
                i--;
        } else {
            vec3 diffuseDir = normalize(hitRecord.normal + (2*texture(u_RgbNoise, seed-vec2(0.062, 0.0345)).xyz-1)*material.roughness);
            vec3 specularDir = reflect(ray.direction, hitRecord.normal);
            bool isSpecular = texture(u_RgbNoise, seed).y < (1-material.roughness);
            specularDir = mix(specularDir, diffuseDir, material.roughness);
            nextDirection = isSpecular ? specularDir : diffuseDir;
            rayColour *= isSpecular ? mix(material.specularColour, material.colour, material.metallic) : material.colour;
        }
        ray = MakeRay(hitRecord.hitPoint + 1e-4 * nextDirection, nextDirection, 1);
        
        vec3 diff = abs(rayColour - lastColour);
        float distance = diff.x + diff.y + diff.z;
        int inc = int(clamp(u_BounceLimit/4, 0, u_BounceLimit));
        if(distance < 1) {
            i += inc;
        }
        if(distance < 0.4) {
            i += inc;
        } 
        if(distance < 0.2) {
            i += inc;
        }
    }
    return vec3(0);
}

void main()
{
    vec3 rgb = vec3(0.0);
    vec2 uv = gl_FragCoord.xy / screenResolution;
    vec3 previous = texture(u_Accumulation, uv).rgb;
    int iterationsDid = 0;
    vec3 albedo = vec3(0.0);
    vec3 normals = vec3(0.0);
    for(int i=0; i<RAY_COUNT; ++i)
    { 
        vec2 randOffset = texture(u_RgbNoise, u_RandSeed.yz + vec2(0.007*i)).xy;
        Ray initialRay = MakeRay(u_Camera.position, directionToViewport(randOffset), 1.0);
        
        vec3 albedoTmp;
        vec3 newRgb = RayColour(initialRay, previous, albedoTmp, normals, i);
        
        vec3 diff = abs(rgb - newRgb);
        float distance = diff.x + diff.y + diff.z;
        if(i > 0) {
            if(distance < 0.5) {
                i += 1;
            }
            if(distance < 0.2) {
                i += 1;
            }
            if(distance < 0.1) {
                i += 1;
            }
        }
        rgb += newRgb;
        albedo += albedoTmp;
        iterationsDid++;
    }
    float iInv = 1.0 / iterationsDid;
    rgb *= iInv;
    albedo *= iInv;
    color = mix(previous, rgb, 1.0 / float(u_FrameIndex + 1));
}
