#pragma once

#include "Math3D.h"
#include "Tri.h"
#include <vector>
#include <algorithm>
#include <limits.h>
#include <cfloat>
#include <chrono>

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

#define DEFAULT_LEAF_TRIANGLES 3
/**
 * bounding volume heirachy tree that takes in a list of vectors belonging to some object and outputs the 
 */
class BvhTree
{
private:
    std::vector<BoundingBox> boundingBoxes;
    std::vector<Tri> triangles;
    int maxDepth;
    int maxTrianglesPerLeaf; // configurable threshold for when to stop subdividing
    unsigned long int numberOfsplitsTotal;
    unsigned long int numberOfDegenerateSplits;

    enum Dimension {
        x = 0,
        y,
        z
    };

    static const std::vector<float> splitRatios;

    std::pair<Dimension, float> SplitLongestDimension(const BoundingBox& box);

    float SurfaceArea(const Vector3f& extent);

    float SurfaceAreaScoreOfSplit(std::vector<Tri>::iterator l, std::vector<Tri>::iterator r, std::vector<Tri>::iterator midIdx);
    
    std::pair<Dimension, float> SplitBest(std::vector<Tri>::iterator l, std::vector<Tri>::iterator r, const BoundingBox& box);

    std::pair<Vector3f, Vector3f> GetBoundingBoxOfRange(std::vector<Tri>::iterator l, std::vector<Tri>::iterator r);

    std::vector<Tri>::iterator PartitionRange(std::vector<Tri>::iterator lIter, std::vector<Tri>::iterator rIter, Dimension splitDimension, float splitValue);

    // make bounding boxes for l and r (recursively)
    // returns the index of the box made in the boxes container, for the current level it is equal to the size - 1 before left and right have been explored (because they add more child boxes)
    unsigned int leafDepthSum;
    unsigned int leafNodescount;
    
    int MakeBox(std::vector<Tri>::iterator l, std::vector<Tri>::iterator r, int currDepth = 1);

public:
    BvhTree() : maxDepth(0), numberOfsplitsTotal(0), numberOfDegenerateSplits(0), maxTrianglesPerLeaf(DEFAULT_LEAF_TRIANGLES) {}
    BvhTree(std::vector<Tri> triangles, int maxTrianglesPerLeaf=DEFAULT_LEAF_TRIANGLES) : triangles(std::move(triangles)), maxDepth(0), numberOfsplitsTotal(0), numberOfDegenerateSplits(0), maxTrianglesPerLeaf(maxTrianglesPerLeaf) {}

    void SetTriangles(std::vector<Tri> newTriangles);

    int GetMaxTrianglesPerLeaf() const {
        return maxTrianglesPerLeaf;
    }

    std::pair<std::vector<BoundingBox>, std::vector<Tri>> BuildTree();
};