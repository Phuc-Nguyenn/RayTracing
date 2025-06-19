#version 420 core

out vec3 color;

#define INF 1.0/0.0
#define MAX_HITTABLE_COUNT 8
#define MAX_OBJ_COUNT 256
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
    int materialIndex;
};

struct BoundingBox {
    vec3 maxi;
    vec3 mini;
    int leftChildIndex;
    int rightChildIndex;
    int triangleStartIndex;
    int triangleCount;
}

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

uniform samplerBuffer u_Triangles;
uniform uint u_TrianglesCount;

uniform samplerBuffer u_BoundingBoxes;
uniform uint u_BoundingBoxesCount;

#define MAX_MATERIALS_COUNT 16
uniform Material u_Materials[MAX_MATERIALS_COUNT];
uniform uint u_MaterialsCount;


struct Object { // represents a grouping of triangles
    vec3 position;
    float scale;
    int trianglesStartIndex;
    int trianglesCount;
    Material material;
};

uniform int u_ObjectsCount;
uniform Object u_Objects[MAX_OBJ_COUNT];
// skybox
uniform samplerCube skybox;

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
    return true;
};

Triangle getTriangle(int index) {
    Triangle triangle;
    int bufferIndex = index*3;
    triangle.position = texelFetch(u_Triangles, bufferIndex).xyz;
    triangle.position2 = texelFetch(u_Triangles, bufferIndex + 1).xyz;
    triangle.position3 = texelFetch(u_Triangles, bufferIndex + 2).xyz;
    return triangle;
}

BoundingBox getBoundingBox(int index) {
    BoundingBox box;
    int bufferIndex = index * 10;
    box.maxi = vec3(
        texelFetch(u_BoundingBoxes, bufferIndex + 0).x,
        texelFetch(u_BoundingBoxes, bufferIndex + 1).x,
        texelFetch(u_BoundingBoxes, bufferIndex + 2).x
    );
    box.mini = vec3(
        texelFetch(u_BoundingBoxes, bufferIndex + 3).x,
        texelFetch(u_BoundingBoxes, bufferIndex + 4).x,
        texelFetch(u_BoundingBoxes, bufferIndex + 5).x
    );
    box.leftChildIndex = int(texelFetch(u_BoundingBoxes, bufferIndex + 6).x);
    box.rightChildIndex = int(texelFetch(u_BoundingBoxes, bufferIndex + 7).x);
    box.triangleStartIndex = int(texelFetch(u_BoundingBoxes, bufferIndex + 8).x);
    box.triangleCount = int(texelFetch(u_BoundingBoxes, bufferIndex + 9).x);
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

HitRecord BoundingBoxHit(BoundingBox)


#define VISUALISE_CLUSTERS false

bool HitHittableList(Ray ray, inout HitRecord hitRecord) {
    hitRecord.t = INF;
    hitRecord.hitAnything = false;

    for(uint i = 0; i < u_HittablesCount; ++i) {
        Hittable hittable = u_Hittable[i];
        HitRecord hitRecordTmp;
        if(hittable.type == SPHERE && hitSphere(Sphere(hittable.position, hittable.scale), ray, hitRecordTmp)) {
            if(hitRecord.t > hitRecordTmp.t) {
                hitRecord = hitRecordTmp;
                hitRecord.material = hittable.material;
                hitRecord.hitAnything = true;
            }
        }
    }

    // HitRecord boundingSphereHit;
    // for(int j = 0; j < u_ObjectsCount; j++) {
    //     if(hitSphere(Sphere(u_Objects[j].position, u_Objects[j].scale), ray, boundingSphereHit)) {
    //         if(VISUALISE_CLUSTERS && hitRecord.t > boundingSphereHit.t) {
    //             hitRecord = boundingSphereHit;
    //             hitRecord.material = u_Objects[j].material;
    //             hitRecord.hitAnything = true;
    //         }
    //     } else {
    //         continue;
    //     }
        
    //     if(VISUALISE_CLUSTERS) 
    //         continue;

    //     if(hitRecord.t < boundingSphereHit.t && boundingSphereHit.frontFace) {
    //         continue;
    //     }

    // Material material = u_Objects[j].material;

    // int trianglesIndexStart = u_Objects[j].trianglesStartIndex;
    // int trianglesIndexEnd = trianglesIndexStart + u_Objects[j].trianglesCount;

    // for(int i = trianglesIndexStart; i < trianglesIndexEnd; ++i) {
    //     Triangle triangle = getTriangle(i);
    //     HitRecord hitRecordTmp;
    //     if(hitTriangle(triangle, ray, hitRecordTmp)) {
    //         if(hitRecord.t > hitRecordTmp.t) {
    //             hitRecord = hitRecordTmp;
    //             hitRecord.material = material;
    //             hitRecord.hitAnything = true;
    //         }
    //     }
    // }
    }

    return hitRecord.hitAnything;
};


#define AIR_REFRACT 1.0003

vec3 TransparentScatter(inout HitRecord hitRecord, inout Material material, inout Ray ray) {
    float ri = hitRecord.frontFace ? AIR_REFRACT/material.refractionIndex : material.refractionIndex/AIR_REFRACT;
    float cos_theta = min(dot(-ray.direction, hitRecord.normal), 1.0);
    float sin_theta = sqrt(1.0 - cos_theta*cos_theta);
    bool canRefract = ri * sin_theta <= 1.0;
    if (canRefract)
        return refract(ray.direction, hitRecord.normal, ri);
    return reflect(ray.direction, hitRecord.normal);
}
#define FOG_DENSITY 0.00
#define FOG_HEIGHT 12.0
 
bool VolumetricScatter(inout HitRecord hitRecord, inout Ray ray, inout vec3 colour, vec2 seed, const float maxFogTravel) {
    // find out if ray will across the fod boundary
    float scatterProbability = 0.0;
    bool scattered = false;

    Ray fogScope = MakeRay(ray.origin, ray.direction, 0.0);
    float distanceToFog = (FOG_HEIGHT - ray.origin.z) / ray.direction.z ;

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
        scattered = texture(u_RgbNoise, seed+vec2(0.012)).x < scatterProbability;
        if(scattered) {
            vec3 newDirection;
            if(hitsLight) 
                newDirection = normalize(ray.direction + texture(u_RgbNoise, seed+vec2(-0.01, 0.034)).rgb * 2 - 1);
            else
                newDirection = normalize(texture(u_RgbNoise, seed-vec2(0.054)).rgb * 2 - 1);
            ray = MakeRay(fogScope.origin + ray.direction * fogScope.magnitude * texture(u_RgbNoise, seed-vec2(0.01, 0.05)).x, newDirection, 1);
            colour *= (1 - FOG_DENSITY);
        }
    }
    return scattered;
}

vec3 RayColour(Ray ray) {
    HitRecord hitRecord;
    vec3 rayColour = vec3(1.0);
    const float maxFogTravel = 1.0/FOG_DENSITY;

    for(int i=0; i<u_BounceLimit; ++i) {
        bool hitAnything = HitHittableList(ray, hitRecord);

        if (VISUALISE_CLUSTERS || u_BounceLimit == 1) {
            return vec3(abs(hitRecord.normal));
        } 

        vec2 seed = mod(vec2(
            Hashf(ray.origin.x + ray.direction.y) + Hash(i) + Hash(u_FrameIndex) + Hashf(u_RandSeed.x * u_BounceLimit),
            Hashf(ray.origin.y + ray.direction.z) + Hash(i+120) + Hashf(u_RandSeed.y * u_BounceLimit)
        ), vec2(100, 100))/ 100;
        
        // handle volumetric scattering
        if (VolumetricScatter(hitRecord, ray, rayColour, seed-vec2(0.31), maxFogTravel)) {
           continue;
        }

        if(!hitAnything) {
            vec3 skybox_texture = texture(skybox, RotateAroundAxis(ray.direction, vec3(1,0,0), -3.1415/2)).rgb;
            return rayColour*skybox_texture;
        } 

        Material material = hitRecord.material;

        if (material.isLight) {
            return rayColour * material.colour;
        }

        vec3 nextDirection;
        if(texture(u_RgbNoise, seed+vec2(0.21, 0.1)).x < material.transparency) {
            nextDirection = TransparentScatter(hitRecord, material, ray);
        } else {
            vec3 diffuseDir = normalize(hitRecord.normal + (2*texture(u_RgbNoise, seed-vec2(0.02, 0.3)).xyz-1)*material.roughness);
            vec3 specularDir = reflect(ray.direction, hitRecord.normal);

            bool isSpecular = texture(u_RgbNoise, seed+vec2(0.03, 0.01)).y < (1-material.roughness);

            specularDir = mix(specularDir, diffuseDir, material.roughness);

            nextDirection = isSpecular ? specularDir : diffuseDir;
            rayColour *= isSpecular ? mix(material.specularColour, material.colour, material.metallic) : material.colour;
        }
        ray = Ray(hitRecord.hitPoint + 1e-4 * nextDirection, nextDirection, 1);
    }
    return vec3(0.1);
}


void main()
{
    vec2 randOffset = texture(u_RgbNoise, u_RandSeed.xy).xy;
    Ray initialRay = Ray(u_Camera.position, directionToViewport(randOffset), 1.0);
    vec3 rgb = RayColour(initialRay);

    vec2 uv = gl_FragCoord.xy / screenResolution;
    vec3 previous = texture(u_Accumulation, uv).rgb;

    color = mix(previous, rgb, 1.0 / float(u_FrameIndex + 1));
}
