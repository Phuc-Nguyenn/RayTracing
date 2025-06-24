#pragma once

#include "Math3D.h"
#include "Shape.h"
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
    int depth;
    int maxTrianglesPerLeaf; // configurable threshold for when to stop subdividing
    unsigned long int numberOfsplitsTotal;
    unsigned long int numberOfDegenerateSplits;



    enum Dimension {
        x = 0,
        y,
        z
    };

    std::pair<Dimension, float> SplitLongestDimension(const BoundingBox& box) { // we seek to split down the long dimension of box
        // find the longest dimension
        Vector3f diff = box.maxi - box.mini;
        if(diff.x >= diff.y && diff.x >= diff.z) {
            return {Dimension::x, box.mini.x + diff.x*0.5}; // split the x
        } else if(diff.y >= diff.x && diff.y >= diff.z) {
            return {Dimension::y, box.mini.y + diff.y*0.5}; // split the y
        }
        return {Dimension::z, box.mini.z + diff.z*0.5}; // split the z
    }

    float SurfaceArea(const Vector3f& extent) {
        return 2.0f * (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
    }

    float surfaceAreaScoreOfSplit(int l, int r, int midIdx) {
        auto [miniL, maxiL] = GetBoundingBoxOfRange(l, midIdx);
        auto [miniR, maxiR] = GetBoundingBoxOfRange(midIdx, r);
        Vector3f extentL = maxiL - miniL;
        Vector3f extentR = maxiR - miniR;

        float areaL = SurfaceArea(extentL);
        float areaR = SurfaceArea(extentR);

        int countL = midIdx - l;
        int countR = r - midIdx;

        return areaL * countL + areaR * countR;
    }

    // splits in half
    std::pair<Dimension, float> SplitBestDimension(int l, int r, const BoundingBox& box) {
        Vector3f splitValue = (box.maxi + box.mini) * 0.5;
        // X Split
        int midIdx = PartitionRange(l, r, Dimension::x, splitValue.x);
        float scoreX = surfaceAreaScoreOfSplit(l, r, midIdx);

        // Y Split
        midIdx = PartitionRange(l, r, Dimension::y, splitValue.y);
        float scoreY = surfaceAreaScoreOfSplit(l, r, midIdx);

        // Z Split
        midIdx = PartitionRange(l, r, Dimension::z, splitValue.z);
        float scoreZ = surfaceAreaScoreOfSplit(l, r, midIdx);
        if (scoreX <= scoreY && scoreX <= scoreZ){
            return {Dimension::x, splitValue.x};
        } else if (scoreY <= scoreZ) {
            return {Dimension::y, splitValue.y};
        } else {
            return {Dimension::z, splitValue.z};
        }
    }

    std::pair<Dimension, float> SplitBestMedianDimension(int l, int r, const BoundingBox& box) {
        Vector3f splitValue = (box.maxi + box.mini) * 0.5;
        // X Split
        float medianX = findMedianOfTris(l, r, Dimension::x);
        int midIdx = PartitionRange(l, r, Dimension::x, medianX);
        float scoreX = surfaceAreaScoreOfSplit(l, r, midIdx);

        // Y Split
        float medianY = findMedianOfTris(l, r, Dimension::y);
        midIdx = PartitionRange(l, r, Dimension::y, medianY);
        float scoreY = surfaceAreaScoreOfSplit(l, r, midIdx);

        // Z Split
        float medianZ = findMedianOfTris(l, r, Dimension::z);
        midIdx = PartitionRange(l, r, Dimension::z, medianZ);
        float scoreZ = surfaceAreaScoreOfSplit(l, r, midIdx);
        if (scoreX <= scoreY && scoreX <= scoreZ){
            return {Dimension::x, medianX};
        } else if (scoreY <= scoreZ) {
            return {Dimension::y, medianY};
        } else {
            return {Dimension::z, medianZ};
        }
    }

    std::pair<Dimension, float> SplitBest(int l, int r, const BoundingBox& box) {
        std::vector<Vector3f> splitTestValues;
        std::vector<float> splitRatios = {0.35, 0.4, 0.425, 0.45, 0.475, 0.5, 0.525, 0.55, 0.575, 0.6, 0.65};
        for(const float& splitRatio : splitRatios) {
            splitTestValues.push_back(box.mini * (1 - splitRatio) + box.maxi * splitRatio);
        }
        std::vector<std::tuple<float, float, Dimension>> results; // (score, value dimension)
        // Loop through every dimension (x, y, z)
        for (int i = 0; i < 3; ++i) {
            Dimension dim = static_cast<Dimension>(i);
            for (auto& splitValue : splitTestValues) {
                int midIdx = PartitionRange(l, r, dim, splitValue[i]);
                if (midIdx == l || midIdx == r) continue;
                float score = surfaceAreaScoreOfSplit(l, r, midIdx);
                results.emplace_back(score, splitValue[i], dim);
            }
        }
        // Choose the best split (lowest score)
        if (!results.empty()) {
            auto bestSplit = *std::min_element(results.begin(), results.end());
            return {std::get<2>(bestSplit), std::get<1>(bestSplit)};
        }
        return SplitLongestDimension(box);
    }

    std::pair<Vector3f, Vector3f> GetBoundingBoxOfRange(int l, int r) {
        Vector3f miniBounds(FLT_MAX, FLT_MAX, FLT_MAX);
        Vector3f maxiBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX); // Fixed: use -FLT_MAX instead of FLT_MIN
        for(int i=l; i<r; ++i) {
            miniBounds.x = std::min(miniBounds.x, triangles[i].mini.x);
            miniBounds.y = std::min(miniBounds.y, triangles[i].mini.y);
            miniBounds.z = std::min(miniBounds.z, triangles[i].mini.z);
            maxiBounds.x = std::max(maxiBounds.x, triangles[i].maxi.x);
            maxiBounds.y = std::max(maxiBounds.y, triangles[i].maxi.y);
            maxiBounds.z = std::max(maxiBounds.z, triangles[i].maxi.z);
        }
        return {miniBounds, maxiBounds};
    }

    float findMedianOfTris(int l, int r, Dimension dim) {
        auto lIter = triangles.begin();
        auto nthIter = triangles.begin();
        std::advance(nthIter, (l + r)/2);
        auto rIter = triangles.begin();
        std::advance(lIter, l);
        std::advance(rIter, r);
        std::nth_element(lIter, nthIter, rIter, [&](const Tri& left, const Tri& right){
            switch(dim) {
                case Dimension::x:
                    return left.Centroid().x < right.Centroid().x;
                case Dimension::y:
                    return left.Centroid().y < right.Centroid().y;
                case Dimension::z:
                    return left.Centroid().z < right.Centroid().z;
                default:
                    return false;
            }
            return false;
        });
        if(dim == Dimension::x) return nthIter->Centroid().x;
        if(dim == Dimension::y) return nthIter->Centroid().y;
        return nthIter->Centroid().z;
    }

    int PartitionRange(int l, int r, Dimension splitDimension, float splitValue) {
        auto lIter = triangles.begin();
        auto rIter = triangles.begin();
        std::advance(lIter, l);
        std::advance(rIter, r);
        auto partitionPoint = std::partition(lIter, rIter, [&](const Tri& triangle) {
            Vector3f centroid = triangle.Centroid();
            if (splitDimension == Dimension::x) {
                return centroid.x < splitValue;
            } else if (splitDimension == Dimension::y) {
                return centroid.y < splitValue;
            } else { // Dimension::z
                return centroid.z < splitValue;
            }
        });
        int midIdx = std::distance(triangles.begin(), partitionPoint);
        return midIdx;
    }

    // make bounding boxes for l and r (recursively)
    // returns the index of the box made in the boxes container, for the current level it is equal to the size - 1 before left and right have been explored (because they add more child boxes)
    int MakeBox(int l, int r, int currDepth = 1) {
        
        // Base case: empty range
        if (l >= r) {
            std::cout << "Warning: empty range [" << l << ", " << r << ")" << std::endl;
            return -1;
        }
        // create a new bounding box for all triangles in current box [l,r)
        auto[mini, maxi] = GetBoundingBoxOfRange(l, r);
        // std::cout << "processing [" << l << ", " << r << ") with " << (r - l) << " triangles" << std::endl;
        BoundingBox& newBox = boundingBoxes.emplace_back(maxi, mini); // add to bounding boxes container and get the index
        int myIndex = boundingBoxes.size() - 1;
        
        if(r - l > maxTrianglesPerLeaf) {
            depth = std::max(depth, currDepth);
            // partition int [left] [overlap] [right]
            // auto[splitDimension, splitValue] = SplitLongestDimension(newBox);
            // auto[splitDimension, splitValue] = SplitBestDimension(l, r, newBox);
            // auto[splitDimension, splitValue] = SplitBestMedianDimension(l, r, newBox);
            auto[splitDimension, splitValue] = SplitBest(l, r, newBox);
            int midIdx = PartitionRange(l, r, splitDimension, splitValue);
            // prevent degenerate
            if(midIdx == l || midIdx == r) {
                midIdx = (l + r) / 2;
                numberOfDegenerateSplits++;
            }
            MakeBox(l, midIdx, currDepth + 1); // left child index is myindex + 1
            boundingBoxes[myIndex].rightChildIndex = MakeBox(midIdx, r, currDepth + 1);
            numberOfsplitsTotal++;
        } else {
            // Leaf node - can contain multiple triangles
            // std::cout << "Creating leaf node with " << (r - l) << " triangles at indices [" << l << ", " << r << ")" << std::endl;
            boundingBoxes[myIndex].triangleStartIndex = l;
            boundingBoxes[myIndex].triangleCount = r - l;
        }
        return myIndex;
    };

public:
    BvhTree() : maxTrianglesPerLeaf(DEFAULT_LEAF_TRIANGLES), depth(0), numberOfsplitsTotal(0), numberOfDegenerateSplits(0) {}
    BvhTree(std::vector<Tri> triangles, int maxTrianglesPerLeaf=DEFAULT_LEAF_TRIANGLES) : triangles(std::move(triangles)), maxTrianglesPerLeaf(maxTrianglesPerLeaf), depth(0), numberOfsplitsTotal(0), numberOfDegenerateSplits(0) {}

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

    int GetDepth() const {
        return depth;
    }

    std::pair<std::vector<BoundingBox>, std::vector<Tri>> BuildTree() {
        if (triangles.empty()) {
            std::cout << "Warning: no triangles to build tree from" << std::endl;
        } else {
            auto begin = std::chrono::steady_clock::now();
            boundingBoxes.clear(); // Clear previous tree
            MakeBox(0, triangles.size());
            auto end = std::chrono::steady_clock::now();
            std::cout << "constructing BVH structure took: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms" << std::endl;
            std::cout << "number of total splits: " << numberOfsplitsTotal << std::endl;
            std::cout << "number of degenerate splits: " << numberOfDegenerateSplits << ", as a percentage of total: " << float(numberOfDegenerateSplits)/numberOfsplitsTotal << std::endl;
        }
        return {boundingBoxes, triangles};
    }
};