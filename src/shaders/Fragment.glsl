#version 420 core

out vec3 color;

#define INF 1.0/0.0
#define BOUNCE_LIMIT 16
#define MAX_OBJ_COUNT 100
#define SPHERE 0
#define PLANE 1

struct Material {
    vec3 colour;
    vec3 specularColour;
    float roughness;
    float transparency;
    float refractionIndex;
    bool isLight;
};

struct Hittable {
    uint type;
    vec3 position;
    vec3 direction;
    float scale;   
    Material material;
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
    float magnitude;
};

uniform vec2 screenResolution;
uniform Camera u_Camera;
uniform Hittable u_Hittable[MAX_OBJ_COUNT];
uniform uint u_HittablesCount;

// for progressive rendering
uniform uint u_FrameIndex;
uniform vec3 u_RandSeed;
uniform sampler2D u_Accumulation;

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

// Returns a float in [0.0, 1.0) from a uint
float Rand(uint m) {
    const uint MantissaMask = 0x007FFFFFu;
    const uint OneFloatBits = 0x3F800000u;
    m = Hash(m) & MantissaMask;         // Hash and extract fractional bits
    m |= OneFloatBits;                  // Combine with base float (1.0)
    float f = uintBitsToFloat(m);       // Get float in [1.0, 2.0)
    return f - 1.0;                     // Shift to [0.0, 1.0)
}

float Randf(float m) {
    return Rand(floatBitsToUint(m));
}

// Generates a pseudo-random vec3 from a seed vector
vec3 MakeRandVec(vec3 seed) {
    return vec3(
        Rand(floatBitsToUint(seed.x)),
        Rand(floatBitsToUint(seed.y)),
        Rand(floatBitsToUint(seed.z))
    );
}

vec3 MakeRandUnitVec(vec3 seed) {
    vec3 try;
    for(uint i=0; i<4; ++i) {
        float tryLen = length(try);
        if(1e-5 < tryLen && tryLen <= 1)
            return try/sqrt(tryLen);
        seed += seed;
        try = MakeRandVec(seed);
    }
    return normalize(try);
}

struct HitRecord {
    vec3 hitPoint;
    vec3 normal;
    float t;
    bool frontFace;
    uint HittableIndex;
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

bool hitSphere(Hittable sphere, Ray r, inout HitRecord hitRecord) {
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


bool HitHittableList(Ray ray, inout HitRecord hitRecord) {
    bool hitAnything = false;
    hitRecord.t = INF;

    for(uint i = 0; i < u_HittablesCount; ++i) {
        Hittable obj = u_Hittable[i];
        HitRecord hitRecordTmp;
        if(obj.type == SPHERE && hitSphere(obj, ray, hitRecordTmp)) {
            if(hitRecord.t > hitRecordTmp.t) {
                hitRecord = hitRecordTmp;
                hitRecord.HittableIndex = i;
                hitAnything = true;
            }
        }
    }
    return hitAnything;
};


#define AIR_REFRACT 1.0003

vec3 TransparentScatter(inout HitRecord hitRecord, inout Hittable object, inout Ray ray) {
    float ri = hitRecord.frontFace ? AIR_REFRACT/object.material.refractionIndex : object.material.refractionIndex/AIR_REFRACT;
    float cos_theta = min(dot(-ray.direction, hitRecord.normal), 1.0);
    float sin_theta = sqrt(1.0 - cos_theta*cos_theta);
    bool canRefract = ri * sin_theta <= 1.0;
    if (canRefract)
        return refract(ray.direction, hitRecord.normal, ri);
    return reflect(ray.direction, hitRecord.normal);

}

vec3 RayColour(Ray ray) {
    HitRecord hitRecord;
    vec3 rayColour = vec3(1.0);
    for(int i=0; i<BOUNCE_LIMIT; ++i) {
        bool hitAnything = HitHittableList(ray, hitRecord);
        
        if(!hitAnything) {
            vec3 skybox_texture = texture(skybox, RotateAroundAxis(ray.direction, vec3(1,0,0), -3.1415/2)).rgb;
            return rayColour*skybox_texture;
        }
        Hittable object = u_Hittable[hitRecord.HittableIndex];
        
        Material material = object.material;
        if (material.isLight) {
            return rayColour*material.colour;
        }

        vec3 nextDirection;
        if(material.transparency > 0.0) {
            nextDirection = TransparentScatter(hitRecord, object, ray);
        } else {
            // Use big prime numbers to decorrelate the random seed for each bounce
            float specularSeed = 
                Hashf(dot(hitRecord.hitPoint, vec3(12.9898, 78.233, 37.719))) +
                Hashf(dot(hitRecord.normal, vec3(39.3468, 11.135, 83.155))) +
                Hash(u_FrameIndex) +
                Hash(i) +
                Hashf(dot(u_RandSeed, vec3(53.123, 41.456, 19.789)));
            
            vec3 diffuseDir = normalize(hitRecord.normal + MakeRandUnitVec(hitRecord.hitPoint)*material.roughness);
            vec3 specularDir = reflect(ray.direction, hitRecord.normal);

            bool isSpecular = Randf(specularSeed) < (1-material.roughness);

            specularDir = mix(specularDir, diffuseDir, material.roughness);

            nextDirection = isSpecular ? specularDir : diffuseDir;
            rayColour *= isSpecular ? material.specularColour : material.colour;
        }
        vec3 offset = 1e-3 * nextDirection;
        ray = Ray(hitRecord.hitPoint + offset, normalize(nextDirection), 1);
    }
    return rayColour;
}


void main()
{
    vec2 randOffset = MakeRandVec(u_RandSeed).xy;
    Ray initialRay = Ray(u_Camera.position, directionToViewport(randOffset), 1.0);
    vec3 rgb = RayColour(initialRay);

    vec2 uv = gl_FragCoord.xy / screenResolution;
    vec3 previous = texture(u_Accumulation, uv).rgb;

    color = mix(previous, rgb, 1.0 / float(u_FrameIndex + 1));
}
