
#pragma once
#include "Math3D.h"

namespace Material
{
struct Material {  
    Vector3f colour;
    Vector3f specularColour;
    float roughness;
    float metallic;
    float transparency;
    float refractionIndex;
    float isLight;

    Material(Vector3f&& colour, Vector3f specularColour = Vector3f(1.0, 1.0, 1.0), float roughness = 1.0, float metallic = 0.0, float transparency = 0.0, float refractionIndex = 0.0, bool isLight = false) :
        colour(colour),
        specularColour(specularColour),
        roughness(roughness),
        metallic(metallic),
        transparency(transparency),
        refractionIndex(refractionIndex),
        isLight(isLight) {};
};

struct Lambertian : Material {
    Lambertian(Vector3f colour) : Material(std::move(colour)){  
    };
};

struct Specular : Material {
    Specular(Vector3f colour, float roughness) : Material(std::move(colour), Vector3f(1.0, 1.0, 1.0), roughness)  {
    };
};

struct Metallic : Material {
    Metallic(Vector3f colour, float roughness, float metallic) : Material(std::move(colour), Vector3f(1.0, 1.0, 1.0), roughness, metallic) {
    };
};

struct Transparent : Material {
    Transparent(Vector3f colour, float transparency, float refractionIndex) : Material(std::move(colour), Vector3f(1.0,1.0,1.0), 0.0, 0.5, transparency,  refractionIndex) {
    };
};

struct LightSource : Material {
    LightSource(Vector3f colour) : Material(std::move(colour), Vector3f(1.0, 1.0, 1.0), 0.0, 0.0, 0.0, 0.0, true) {
    }
};
}; // namespace Material