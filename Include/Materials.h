
#pragma once
#include "Math3D.h"

namespace Material
{
struct Material {  
    Vector3f colour;
    Vector3f specularColour;
    float roughness;
    float transparency;
    float refractionIndex;
    bool isLight;

    Material(Vector3f&& colour, Vector3f specularColour = Vector3f(1.0, 1.0, 1.0), float roughness = 1.0, float transparency = 0.0, float refractionIndex = 0.0, bool isLight = false) :
        colour(colour),
        specularColour(specularColour),
        roughness(roughness),
        transparency(transparency),
        refractionIndex(refractionIndex),
        isLight(isLight) {};
};

struct Lambertian : Material {
    Lambertian(Vector3f colour) : Material(std::move(colour)){  
    };
};

struct Reflective : Material {
    Reflective(Vector3f colour, float reflectivity, float roughness) : Material(std::move(colour), Vector3f(1.0, 1.0, 1.0), reflectivity, roughness)  {
    };
};

struct Transparent : Material {
    Transparent(Vector3f colour, float transparency, float refractionIndex) : Material(std::move(colour), Vector3f(1.0,1.0,1.0), 0.0, transparency,  refractionIndex) {
    };
};

struct LightSource : Material {
    LightSource(Vector3f colour) : Material(std::move(colour), Vector3f(1.0, 1.0, 1.0), 0.0, 0.0, 0.0, true) {
    }
};
}; // namespace Material