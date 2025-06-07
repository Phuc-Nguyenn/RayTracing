#pragma once

#include "Math3D.h"
#include "Materials.h"
#include <string>
#include <memory>

class Object
{
protected:
    Vector3f position;
    Vector3f dir;
    float scale;
    Material::Material material;

public:
    Object(Vector3f position,
           Vector3f dir,
           float scale,
           Material::Material material,
           bool isLight = false)
        : position(position),
          dir(dir.Normalize()),
          scale(scale),
          material(material)
    {}

    virtual ~Object() = default;

    virtual unsigned int getType() const = 0;

    const Vector3f& getPosition() const {
        return position;
    }

    void setPosition(const Vector3f& pos) {
        position = pos;
    }

    const Vector3f& getDir() const {
        return dir;
    }

    void setDir(Vector3f direction) {
        dir = direction.Normalize();
    }

    float getScale() const {
        return scale;
    }

    void setScale(float s) {
        scale = s;
    }

    const Material::Material& getMaterial() const {
        return material;
    }

    void setMaterial(const Material::Material& mat) {
        material = mat;
    }
};

class Sphere : public Object
{
private:
    static const unsigned int type = 0;

public:
    Sphere(Vector3f position, float radius, Material::Material material, bool isLight = false)
        : Object(position, {0.0f, 0.0f, 0.0f}, radius, material, isLight) {}

    unsigned int getType() const override {
        return type;
    }
};