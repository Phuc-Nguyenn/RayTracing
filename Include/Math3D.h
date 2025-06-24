#pragma once

#include <cmath>
#include <iostream>

struct Vector2f {
    union {
        float x = 0.0f;
        float r;
    };
    union {
        float y = 0.0f;
        float g;
    };

    Vector2f() {
        x = 0.0f; y = 0.0f;
    };

    Vector2f(float _x, float _y) {
        x = _x;
        y = _y;
    };
};

struct Vector3f {
    union {
        float x = 0.0f;
        float r;
    };
    union {
        float y = 0.0f;
        float g;
    };
    union {
        float z = 0.0f;
        float b;
    };

    Vector3f() {
        x = 0.0f; y = 0.0f; z = 0.0f;
    };

    Vector3f(float _x, float _y, float _z) {
        x = _x;
        y = _y;
        z = _z;
    };

    Vector3f Normalize() {
        float mag = sqrt(x * x + y * y + z * z);
        return Vector3f{
            x / mag,
            y / mag,
            z / mag
        };
    };

    Vector3f Cross(const Vector3f& v) const {
        return Vector3f(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    };

    Vector3f operator*(float f) const {
        return Vector3f(
            x * f,
            y * f,
            z * f
        );
    };

    Vector3f operator*(Vector3f v) const {
        return Vector3f(
            x * v.x,
            y * v.y,
            z * v.z
        );
    };

    Vector3f operator/(float div) const {
        return Vector3f(
            x / div,
            y / div,
            z / div
        );
    };

    Vector3f operator+(const Vector3f& v) const {
        return Vector3f(
            x + v.x,
            y + v.y,
            z + v.z
        );
    };

    Vector3f operator-(const Vector3f& v) const {
        return Vector3f(
            x - v.x,
            y - v.y,
            z - v.z
        );
    };

    float& operator[](int index) {
        if(index == 0) return x;
        if(index == 1) return y;
        if(index == 2) return z;
        std::cout << "illegal access of Vector3f at index " << index << std::endl;
        std::abort();
    }

    float len() const {
        return sqrt(x*x + y*y + z*z);
    }

    float Dot(const Vector3f& v) const {
        return x*v.x + y*v.y + z*v.z;
    }

    void Print() {
        std::cout << "{" << x << ", " << y << ", " << z << "}" << std::endl;
    }

    bool operator==(const Vector3f& v) const {
        return x==v.x && y==v.y && z==v.z;
    }
};

