#pragma once

#include "Math3D.h"
#include "Shape.h"
#include <vector>
#include <algorithm>
#include <limits.h>
#include <cfloat>

class BoundingBox {
public:
    Vector3f maxi;
    Vector3f mini;
    int leftChildIndex; // added for completeness but would be the next index in the vector
    int rightChildIndex;
    int triangleIndex;

    BoundingBox() = default;

    BoundingBox(Vector3f maxi, Vector3f mini, int leftChildIndex = -1, int rightChildIndex = -1, int triangleIndex = -1)
    : maxi(maxi), mini(mini), rightChildIndex(rightChildIndex), triangleIndex(triangleIndex) {
    }
};

/**
 * bounding volume heirachy tree that takes in a list of vectors belonging to some object and outputs the 
 */
class BvhTree
{
private:
    std::vector<BoundingBox> boundingBoxes;
    std::vector<Tri> triangles;

    enum Dimension {
        x = 0,
        y,
        z
    };

    std::pair<Dimension, float> SplitLongestDimension(const BoundingBox& box) { // we seek to split down the long dimension of box
        // find the longest dimension
        Vector3f diff = box.maxi - box.mini;
        if(diff.x >= diff.y && diff.x >= diff.z) {
            return {Dimension::x, box.mini.x + diff.x/2}; // split the x
        } else if(diff.y >= diff.x && diff.y >= diff.z) {
            return {Dimension::y, box.mini.y + diff.y/2}; // split the y
        }
        return {Dimension::z, box.mini.z + diff.z/2}; // split the z
    }

    // make bounding boxes for l and r (recursively)
    // returns the index of the box made in the boxes container, for the current level it is equal to the size - 1 before left and right have been explored (because they add more child boxes)
    int MakeBox(int l, int r) {
        
        // create a new bounding box for all triangles
        BoundingBox newBox;
        Vector3f miniBounds(FLT_MAX, FLT_MAX, FLT_MAX);
        Vector3f maxiBounds(FLT_MIN, FLT_MIN, FLT_MIN);
        for(int i=l; i<r; ++i) {
            // get the mini and maxi bounds
            const Vector3f verts[3] = { triangles[i].pos1, triangles[i].pos2, triangles[i].pos3 };
            for (int j = 0; j < 3; ++j) {
                miniBounds.x = std::min(miniBounds.x, verts[j].x);
                miniBounds.y = std::min(miniBounds.y, verts[j].y);
                miniBounds.z = std::min(miniBounds.z, verts[j].z);
                maxiBounds.x = std::max(maxiBounds.x, verts[j].x);
                maxiBounds.y = std::max(maxiBounds.y, verts[j].y);
                maxiBounds.z = std::max(maxiBounds.z, verts[j].z);
            }
        }
        newBox.mini = miniBounds;
        newBox.maxi = maxiBounds;

        // partition int [left] [overlap] [right]
        std::pair<Dimension, float> splitResult = SplitLongestDimension(newBox);
        auto lIter = triangles.begin();
        auto rIter = triangles.begin();
        std::advance(lIter, l);
        std::advance(rIter, r);
        float splitValue = splitResult.second;
        int midIdx = std::distance(triangles.begin(), std::partition(lIter, rIter, [&](const Tri& triangle) {
            Vector3f centroid = triangle.Centroid();
            if (splitResult.first == Dimension::x) {
            return centroid.x < splitValue;
            } else if (splitResult.first == Dimension::y) {
            return centroid.y < splitValue;
            } else { // Dimension::z
            return centroid.z < splitValue;
            }
        }));

        std::cout << "processing [" << l << ", " << r << ")" << std::endl;
        boundingBoxes.push_back(newBox); // add to bounding boxes container and get the index
        int myIndex = boundingBoxes.size() - 1;
        if(r - l > 1) {
            boundingBoxes[myIndex].leftChildIndex = MakeBox(l, midIdx);
            boundingBoxes[myIndex].rightChildIndex = MakeBox(midIdx, r);
        } else {
            if(r - l != 1) {
                std::cout << "expected leaf node l - r did not equal 1, instead equals: " << l - r << std::endl;
            } else {
                boundingBoxes[myIndex].triangleIndex = l;
            }
        }
        return myIndex;
    };

public:
    BvhTree() = default;
    BvhTree(std::vector<Tri> triangles) : triangles(triangles) {}

    void SetTriangles(std::vector<Tri> triangles) {
        triangles = triangles;
    }

    std::vector<BoundingBox> BuildTree() {
        MakeBox(0, triangles.size());
        return boundingBoxes;
    }


};