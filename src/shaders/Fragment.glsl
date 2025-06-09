#version 420 core

out vec3 color;

#define INF 1.0/0.0
#define BOUNCE_LIMIT 4
#define MAX_OBJ_COUNT 100
#define SPHERE 0
#define PLANE 1

struct Material {
    vec3 colour;
    vec3 specularColour;
    float roughness;
    float metallic;
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
uniform sampler2D u_GrayNoise;
uniform sampler2D u_RgbNoise;

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

#define FOG_DENSITY 0.025
#define FOG_HEIGHT 128.0

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

    fogScope.magnitude = min(fogScope.magnitude, maxFogTravel);
    if (fogScope.magnitude > 0.0 && fogScope.magnitude <= hitRecord.t) {
        // probability of scattering is proportional to the distance traveled in fog
        scatterProbability = FOG_DENSITY * fogScope.magnitude;
        scattered = texture(u_GrayNoise, seed).x < scatterProbability;
        if(scattered) {
            vec3 newDirection = normalize(texture(u_RgbNoise, seed).rgb * 2 - 1);
            ray = MakeRay(fogScope.origin + ray.direction * fogScope.magnitude * texture(u_GrayNoise, seed+vec2(0.01)).x, newDirection, 1);
            colour *= (1 - FOG_DENSITY/4);
        }
    }
    return scattered;
}

vec3 RayColour(Ray ray) {
    HitRecord hitRecord;
    vec3 rayColour = vec3(1.0);
    const float maxFogTravel = 1.0/FOG_DENSITY;

    for(int i=0; i<BOUNCE_LIMIT; ++i) {
        bool hitAnything = HitHittableList(ray, hitRecord);
        
        vec2 seed = mod(
            abs(
            vec2(Hashf(ray.direction.x), Hashf(ray.direction.y) +
            Hash(i) +
            Hash(u_FrameIndex) +
            vec2(Hashf(hitRecord.hitPoint.x), Hashf(hitRecord.hitPoint.y)) +
            vec2(Hashf(u_RandSeed.x), Hashf(u_RandSeed.y))
            )), vec2(400, 600)
        ) / vec2(400, 600);
        
        // handle volumetric scattering
        if (VolumetricScatter(hitRecord, ray, rayColour, seed, maxFogTravel)) {
           continue;
        }

        if(!hitAnything) {
            vec3 skybox_texture = texture(skybox, RotateAroundAxis(ray.direction, vec3(1,0,0), -3.1415/2)).rgb;
            return rayColour*skybox_texture;
        } 
        
        Hittable object = u_Hittable[hitRecord.HittableIndex];
        Material material = object.material;

        if (material.isLight) {
            return rayColour * material.colour;
        }

        vec3 nextDirection;
        if(texture(u_GrayNoise, seed).x < material.transparency) {
            nextDirection = TransparentScatter(hitRecord, object, ray);
        } else {

            vec3 diffuseDir = normalize(hitRecord.normal + normalize(texture(u_RgbNoise, seed+vec2(0.02)).xyz)*material.roughness);
            vec3 specularDir = reflect(ray.direction, hitRecord.normal);

            bool isSpecular = texture(u_GrayNoise, seed+vec2(0.03)).x < (1-material.roughness);

            specularDir = mix(specularDir, diffuseDir, material.roughness);

            nextDirection = isSpecular ? specularDir : diffuseDir;
            rayColour *= isSpecular ? mix(material.specularColour, material.colour, material.metallic) : material.colour;
        }
        vec3 offset = 1e-3 * nextDirection;
        ray = Ray(hitRecord.hitPoint + offset, normalize(nextDirection), 1);
    }
    return vec3(0.0);
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
