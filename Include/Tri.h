#pragma once

#include "Math3D.h"
#include "Materials.h"
#include <string>
#include <memory>

class Tri {
public:
    Vector3f pos1;
    Vector3f pos2;
    Vector3f pos3;
    Vector3f maxi; // precompute maxi and mini
    Vector3f mini; // precompute maxi and mini
    Vector3f centroid; // precompute centroid
    int materialsIndex;

    Tri(Vector3f pos1, Vector3f pos2, Vector3f pos3, int materialsIndex = 0)
    : pos1(pos1), pos2(pos2), pos3(pos3), materialsIndex(materialsIndex) {
        maxi = Vector3f(std::max(pos1.x, std::max(pos2.x, pos3.x)),
                    std::max(pos1.y, std::max(pos2.y, pos3.y)),
                    std::max(pos1.z, std::max(pos2.z, pos3.z)));
        mini = Vector3f(std::min(pos1.x, std::min(pos2.x, pos3.x)),
                    std::min(pos1.y, std::min(pos2.y, pos3.y)),
                    std::min(pos1.z, std::min(pos2.z, pos3.z)));
        centroid = (pos1 + pos2 + pos3) / 3;
    }

    Vector3f Centroid() const{
        return centroid;
    }
};