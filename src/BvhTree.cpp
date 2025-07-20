#include "BvhTree.h"
#include "ConfigParser.hpp"

#include <optional>

BvhTree::BvhTree() : maxDepth(0), numberOfsplitsTotal(0), numberOfDegenerateSplits(0), maxTrianglesPerLeaf(DEFAULT_LEAF_TRIANGLES), activeThreadCount(1)
{
    ConfigParser& configParser = ConfigParser::GetSingleton();
    maxThreadCount = configParser.aConfig<unsigned int>("BvhTree", "MaxThreadCount");
    threadCreationThreshold = configParser.aConfig<unsigned int>("BvhTree", "ThreadCreationThreshold");
}

BvhTree::BvhTree(std::vector<Tri> triangles, int maxTrianglesPerLeaf) : triangles(std::move(triangles)), maxDepth(0), numberOfsplitsTotal(0), numberOfDegenerateSplits(0), maxTrianglesPerLeaf(maxTrianglesPerLeaf), activeThreadCount(1)
{
    ConfigParser& configParser = ConfigParser::GetSingleton();
    maxThreadCount = configParser.aConfig<unsigned int>("BvhTree", "MaxThreadCount");
    threadCreationThreshold = configParser.aConfig<unsigned int>("BvhTree", "ThreadCreationThreshold");
}

const std::vector<float> BvhTree::splitRatios = {
    0.05f, 0.10f, 0.15f, 0.20f, 0.25f, 0.30f, 0.35f, 0.40f, 0.45f, 0.50f,
    0.55f, 0.60f, 0.65f, 0.70f, 0.75f, 0.80f, 0.85f, 0.90f, 0.95f
};

std::pair<BvhTree::Dimension, float> BvhTree::SplitLongestDimension(const BoundingBox& box) { // we seek to split down the long dimension of box
    // find the longest dimension
    Vector3f diff = box.maxi - box.mini;
    if(diff.x >= diff.y && diff.x >= diff.z) {
        return {BvhTree::Dimension::x, box.mini.x + diff.x*0.5}; // split the x
    } else if(diff.y >= diff.x && diff.y >= diff.z) {
        return {BvhTree::Dimension::y, box.mini.y + diff.y*0.5}; // split the y
    }
    return {BvhTree::Dimension::z, box.mini.z + diff.z*0.5}; // split the z
}

float BvhTree::SurfaceArea(const Vector3f& extent) {
    return 2.0f * (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
}

float BvhTree::SurfaceAreaScoreOfSplit(std::vector<Tri>::iterator l, std::vector<Tri>::iterator r, std::vector<Tri>::iterator midIdx) {
    auto [miniL, maxiL] = GetBoundingBoxOfRange(l, midIdx);
    auto [miniR, maxiR] = GetBoundingBoxOfRange(midIdx, r);
    Vector3f extentL = maxiL - miniL;
    Vector3f extentR = maxiR - miniR;
    float areaL = SurfaceArea(extentL);
    float areaR = SurfaceArea(extentR);
    unsigned int countL = midIdx - l;
    unsigned int countR = r - midIdx;
    return areaL * countL + areaR * countR;
}

std::pair<BvhTree::Dimension, float> BvhTree::SplitBest(std::vector<Tri>::iterator l, std::vector<Tri>::iterator r, const BoundingBox& box) {
    std::vector<Vector3f> splitTestValues;
    std::vector<std::tuple<float, float, Dimension>> splitTrialResults;
    splitTestValues.reserve(splitRatios.size() * 3);
    splitTrialResults.reserve(splitRatios.size() * 3); // (score, value dimension)
    for(const float& splitRatio : splitRatios) {
        splitTestValues.push_back(box.mini * (1 - splitRatio) + box.maxi * splitRatio);
    }
    // Loop through every dimension (x, y, z)
    for (int i = 0; i < 3; ++i) {
        Dimension dim = static_cast<Dimension>(i);
        for (auto& splitValue : splitTestValues) {
            auto midIter = PartitionRange(l, r, dim, splitValue[i]);
            if (midIter == l || midIter == r) continue;
            float score = SurfaceAreaScoreOfSplit(l, r, midIter);
            splitTrialResults.emplace_back(score, splitValue[i], dim);
        }
    }
    if (!splitTrialResults.empty()) { // Choose the best split (lowest score)
        auto bestSplit = *std::min_element(splitTrialResults.begin(), splitTrialResults.end());
        return {std::get<2>(bestSplit), std::get<1>(bestSplit)};
    }
    return SplitLongestDimension(box);
}

std::pair<Vector3f, Vector3f> BvhTree::GetBoundingBoxOfRange(std::vector<Tri>::iterator l, std::vector<Tri>::iterator r) {
    Vector3f miniBounds(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3f maxiBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX); // Fixed: use -FLT_MAX instead of FLT_MIN
    for(auto iter = l; iter != r; iter++) {
        miniBounds.x = std::min(miniBounds.x, iter->mini.x);
        miniBounds.y = std::min(miniBounds.y, iter->mini.y);
        miniBounds.z = std::min(miniBounds.z, iter->mini.z);
        maxiBounds.x = std::max(maxiBounds.x, iter->maxi.x);
        maxiBounds.y = std::max(maxiBounds.y, iter->maxi.y);
        maxiBounds.z = std::max(maxiBounds.z, iter->maxi.z);
    }
    return {miniBounds, maxiBounds};
}

std::vector<Tri>::iterator BvhTree::PartitionRange(std::vector<Tri>::iterator lIter, std::vector<Tri>::iterator rIter, Dimension splitDimension, float splitValue) {
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
    return partitionPoint;
}

#include <thread>
int BvhTree::MakeBox(std::vector<Tri>::iterator l, std::vector<Tri>::iterator r, std::vector<BoundingBox>& myBoundingBoxes, int currDepth) {
    // Base case: empty range
    if (l == r) {
        std::cout << "Warning: empty range" << std::endl;
        return -1;
    }
    // create a new bounding box for all triangles in current box [l,r)
    auto[mini, maxi] = GetBoundingBoxOfRange(l, r);
    BoundingBox newBox = myBoundingBoxes.emplace_back(maxi, mini); // add to bounding boxes container and get the index
    int myIndex = myBoundingBoxes.size() - 1;
    if(r - l > maxTrianglesPerLeaf) {
        maxDepth = std::max(maxDepth, currDepth);
        auto[splitDimension, splitValue] = SplitBest(l, r, newBox);
        auto midIter = PartitionRange(l, r, splitDimension, splitValue);
        // prevent degenerate
        if(midIter == l || midIter == r) {
            midIter = l;
            std::advance(midIter, std::distance(l, r) * 0.5);
            numberOfDegenerateSplits++;
        }

        std::optional<std::thread> t;
        std::vector<BoundingBox> rightBoundingBoxes;
        int rightSubtreeRootIndexInRight = -1;
        
        /** this section decides whether to spawn a thread to deal with the right branch, depending on the number of active threads */
        bool spawnThread = false;
        {
            std::lock_guard<std::mutex> lock(activeThreadCountMutex);
            if(activeThreadCount < maxThreadCount && std::distance(l, r) > threadCreationThreshold)
            {
                ++activeThreadCount;
                spawnThread = true;
            }
        }

        if(spawnThread) {
            t.emplace(std::thread([&, midIter, r, currDepth](){
                rightBoundingBoxes.reserve(std::distance(midIter, r) * 2);
                MakeBox(midIter, r, rightBoundingBoxes, currDepth + 1);
                --activeThreadCount;
            })); 
            MakeBox(l, midIter, myBoundingBoxes, currDepth + 1);
        } else {
            MakeBox(l, midIter, myBoundingBoxes, currDepth + 1);
            myBoundingBoxes[myIndex].rightChildIndex = MakeBox(midIter, r, myBoundingBoxes, currDepth + 1);
        }

        if(t && t->joinable())
        {
            t->join();
            int rightRootOffset = static_cast<int>(myBoundingBoxes.size());
            myBoundingBoxes[myIndex].rightChildIndex = rightRootOffset;
            
            for(auto& bb : rightBoundingBoxes) {
                bb.rightChildIndex += rightRootOffset;
                myBoundingBoxes.emplace_back(std::move(bb));
            }
        }
        numberOfsplitsTotal++;
    } else {
        myBoundingBoxes[myIndex].triangleStartIndex = std::distance(triangles.begin(), l);
        myBoundingBoxes[myIndex].triangleCount = std::distance(l, r);

        leafNodescount++;
    }
    return myIndex;
};

void BvhTree::SetTriangles(std::vector<Tri> newTriangles) {
    triangles = newTriangles;
    boundingBoxes.clear(); // Clear previous tree when setting new triangles
}

std::pair<std::vector<BoundingBox>, std::vector<Tri>> BvhTree::BuildTree() {
    leafDepthSum = 0;
    leafNodescount = 0;
    if (triangles.empty()) {
        std::cout << "Warning: no triangles to build tree from" << std::endl;
        return {boundingBoxes, triangles};
    }

    auto begin = std::chrono::steady_clock::now();
    
    boundingBoxes.clear(); // Clear previous tree
    boundingBoxes.reserve(triangles.size() * 2); // Reserve space for bounding boxes
    
    std::cout << "BvhTree::BuildTree(): building with " << maxThreadCount << " threads and " << threadCreationThreshold << " thread creation threshold." << std::endl;

    MakeBox(triangles.begin(), triangles.end(), boundingBoxes);

    auto end = std::chrono::steady_clock::now();
    std::cout << "constructing BVH structure took: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms" << std::endl;
    std::cout << "number of bounding boxes: " << boundingBoxes.size() + 1 << std::endl;
    std::cout << "number of total splits: " << numberOfsplitsTotal << std::endl;
    std::cout << "number of degenerate splits: " << numberOfDegenerateSplits << ", as a percentage of total: " << float(numberOfDegenerateSplits)/numberOfsplitsTotal << std::endl;
    std::cout << "number of leaf nodes: " << leafNodescount << std::endl;
    std::cout << "maximum depth of leaf nodes " << maxDepth << ", average depth: " << float(leafDepthSum)/leafNodescount << std::endl;

    return {boundingBoxes, triangles};
};