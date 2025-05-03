#include "Math3D.h"

namespace Material
{
struct Material {
    Vector3f colour;
    float reflectivity;
    float roughness;
    bool transparent;
    float refractionIndex;
    bool isLightSource;

    Material(Vector3f&& colour, bool isLightSource = false, float reflectivity = 0.0, float roughness = 0.0, bool transparent = false, float refractionIndex = 0.0) :
        colour(colour),
        reflectivity(reflectivity),
        roughness(roughness),
        transparent(transparent),
        refractionIndex(refractionIndex),
        isLightSource(isLightSource)
    {};
};

struct Lambertian : Material {
    Lambertian(Vector3f colour) : Material(std::move(colour)){  
    };
};

struct Reflective : Material {
    Reflective(Vector3f colour, float reflectivity, float roughness) : Material(std::move(colour), false, reflectivity, roughness)  {
    };
};

struct Transparent : Material {
    Transparent(Vector3f colour, float refractionIndex) : Material(std::move(colour), false, 0.0, 0.0, true, refractionIndex) {
    };
};

struct Metal : Material {
    Metal(Vector3f colour, float roughness) : Material(std::move(colour), false, 0.333, roughness) {
    };
};

struct LightSource : Material {
    LightSource(Vector3f colour) : Material(std::move(colour), true) {};
};
};