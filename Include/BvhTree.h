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
    int rightChildIndex;
    int triangleStartIndex;  // start index of triangles for leaf nodes
    int triangleCount;       // number of triangles for leaf nodes

    BoundingBox()
    : maxi(Vector3f(0,0,0)), mini(Vector3f(0,0,0)), rightChildIndex(0), triangleStartIndex(0), triangleCount(0)
    {}

    BoundingBox(Vector3f maxi, Vector3f mini, int rightChildIndex = -1, int triangleStartIndex = -1, int triangleCount = 0)
    : maxi(maxi), mini(mini), rightChildIndex(rightChildIndex), triangleStartIndex(triangleStartIndex), triangleCount(triangleCount) {
    }

    // Helper methods to check node type
    bool IsLeaf() const { return triangleCount > 0; }
};

#define DEFAULT_LEAF_TRIANGLES 2
/**
 * bounding volume heirachy tree that takes in a list of vectors belonging to some object and outputs the 
 */
class BvhTree
{
private:
    std::vector<BoundingBox> boundingBoxes;
    std::vector<Tri> triangles;
    int maxTrianglesPerLeaf; // configurable threshold for when to stop subdividing

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
        
        // Base case: empty range
        if (l >= r) {
            std::cout << "Warning: empty range [" << l << ", " << r << ")" << std::endl;
            return -1;
        }

        // create a new bounding box for all triangles
        BoundingBox newBox;
        Vector3f miniBounds(FLT_MAX, FLT_MAX, FLT_MAX);
        Vector3f maxiBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX); // Fixed: use -FLT_MAX instead of FLT_MIN
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
        newBox.mini = miniBounds - Vector3f(0.001, 0.001, 0.001);
        newBox.maxi = maxiBounds + Vector3f(0.001, 0.001, 0.001);

        // std::cout << "processing [" << l << ", " << r << ") with " << (r - l) << " triangles" << std::endl;
        boundingBoxes.push_back(newBox); // add to bounding boxes container and get the index
        int myIndex = boundingBoxes.size() - 1;
        
        if(r - l > maxTrianglesPerLeaf) {
            // partition int [left] [overlap] [right]
            std::pair<Dimension, float> splitResult = SplitLongestDimension(newBox);
            auto lIter = triangles.begin();
            auto rIter = triangles.begin();
            std::advance(lIter, l);
            std::advance(rIter, r);
            float splitValue = splitResult.second;
            auto partitionPoint = std::partition(lIter, rIter, [&](const Tri& triangle) {
                Vector3f centroid = triangle.Centroid();
                if (splitResult.first == Dimension::x) {
                    return centroid.x < splitValue;
                } else if (splitResult.first == Dimension::y) {
                    return centroid.y < splitValue;
                } else { // Dimension::z
                    return centroid.z < splitValue;
                }
            });
            int midIdx = std::distance(triangles.begin(), partitionPoint);

            // Prevent degenerate splits that could cause infinite recursion
            if (midIdx == l || midIdx == r) {
                // std::cout << "Warning: degenerate partition detected. All centroids on one side of split." << std::endl;
                // Sort triangles by centroid along the chosen dimension
                std::sort(lIter, rIter, [&](const Tri& a, const Tri& b) {
                    Vector3f centroidA = a.Centroid();
                    Vector3f centroidB = b.Centroid();
                    if (splitResult.first == Dimension::x) return centroidA.x < centroidB.x;
                    else if (splitResult.first == Dimension::y) return centroidA.y < centroidB.y;
                    else return centroidA.z < centroidB.z;
                });
                
                // Split in the middle of the sorted range
                midIdx = l + (r - l) / 2;
                // std::cout << "Sorted split: left=[" << l << "," << midIdx << "), right=[" << midIdx << "," << r << ")" << std::endl;
            }

            MakeBox(l, midIdx); // left child index is myindex + 1
            boundingBoxes[myIndex].rightChildIndex = MakeBox(midIdx, r);
        } else {
            // Leaf node - can contain multiple triangles
            // std::cout << "Creating leaf node with " << (r - l) << " triangles at indices [" << l << ", " << r << ")" << std::endl;
            boundingBoxes[myIndex].triangleStartIndex = l;
            boundingBoxes[myIndex].triangleCount = r - l;
        }
        return myIndex;
    };

public:
    BvhTree() : maxTrianglesPerLeaf(DEFAULT_LEAF_TRIANGLES) {}
    BvhTree(std::vector<Tri> triangles, int maxTrianglesPerLeaf=DEFAULT_LEAF_TRIANGLES) : triangles(std::move(triangles)), maxTrianglesPerLeaf(maxTrianglesPerLeaf) {}

    void SetTriangles(std::vector<Tri> newTriangles) {
        this->triangles = std::move(newTriangles); // Fixed: assign to member variable
        boundingBoxes.clear(); // Clear previous tree when setting new triangles
    }

    void SetMaxTrianglesPerLeaf(int maxTriangles) {
        maxTrianglesPerLeaf = std::max(1, maxTriangles); // Ensure at least 1
    }

    int GetMaxTrianglesPerLeaf() const {
        return maxTrianglesPerLeaf;
    }

    std::pair<std::vector<BoundingBox>, std::vector<Tri>> BuildTree() {
        if (triangles.empty()) {
            std::cout << "Warning: no triangles to build tree from" << std::endl;
        } else {
            boundingBoxes.clear(); // Clear previous tree
            MakeBox(0, triangles.size());
        }
        return {boundingBoxes, triangles};
    }
};