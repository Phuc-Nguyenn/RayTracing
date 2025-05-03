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

    Vector3f& Normalize() {
        float mag = sqrt(x * x + y * y + z * z);
        if (mag > 0.0f) {
            x /= mag;
            y /= mag;
            z /= mag;
        }
        return *this;
    }

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

