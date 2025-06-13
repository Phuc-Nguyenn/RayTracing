#pragma once

#include "Math3D.h"
#include "Materials.h"
#include <string>
#include <memory>

class Shape
{
protected:
    Vector3f position;
    Vector3f position2;
    Vector3f position3;
    Vector3f dir;
    Material::Material material;
    float scale;

public:
    Shape(Vector3f position, Vector3f position2, Vector3f position3,
           Vector3f dir,
           float scale,
           Material::Material material,
           bool isLight = false)
        : position(position), position2(position2), position3(position3),
          dir(dir.Normalize()),
          scale(scale),
          material(material)
    {}

    virtual ~Shape() = default;

    virtual unsigned int getType() const = 0;

    const Vector3f& getPosition() const {
        return position;
    }
    const Vector3f& getPosition2() const {
        return position2;
    }
    const Vector3f& getPosition3() const {
        return position3;
    }

    void setPosition(const Vector3f& pos) {
        position = pos;
    }
    void setPosition2(const Vector3f& pos) {
        position2 = pos;
    }
    void setPosition3(const Vector3f& pos) {
        position3 = pos;
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

class Sphere : public Shape
{
private:
    static const unsigned int type = 0;

public:
    Sphere(Vector3f position, float radius, Material::Material material, bool isLight = false)
        : Shape(position, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0f, 0.0f, 0.0f}, radius, material, isLight) {}

    unsigned int getType() const override {
        return type;
    }
};

class Triangle : public Shape
{
private:
    static const unsigned int type = 1;

public:
    Triangle(Vector3f v1, Vector3f v2, Vector3f v3, Material::Material material, bool isLight = false)
        : Shape(v1, v2, v3, ((v2 - v1).Cross(v3 - v1)).Normalize(), 1.0f, material, isLight) {}

    unsigned int getType() const override {
        return type;
    }
};
