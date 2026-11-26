#include "loaders/cpu_bvh_build_morton_loader.hpp"

#include <bitset>
#include <utility>
#include <bit>
#include <unordered_map>
#include <cstdlib>
#include <set>

namespace Prism::Loaders
{
    namespace
    {
        using AABB    = Resources::CPUBVHResource::AABB;
        using BVHNode = Resources::CPUBVHResource::BVHNode;
        using BVHLeaf = Resources::CPUBVHResource::BVHLeaf;

        constexpr auto MAX_BVH_CHILDREN = 6;

        uint32_t convertTriangleToMortonCode(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec4 sceneAABBLower, glm::vec4 sceneAABBUpper)
        {
            glm::vec3 centroid = (v0 + v1 + v2) / 3.0f;

            constexpr float binCount = 1024; // we have 10 bits.

            // First normalize centroid according to scene bounding box. After that, it is in scene space. Range: <0, 1>
            const glm::vec3 normalized = glm::clamp((centroid - glm::vec3(sceneAABBLower)) / glm::vec3(sceneAABBUpper - sceneAABBLower), 0.0f, 1.0f);

            // Quantize scene space
            const glm::uvec3 quantized = glm::min(normalized * binCount, binCount - 1.0f);

            // intertwine bits like that:
            // zyx

            uint32_t code = 0;

            constexpr auto bitsPerAxis = 10;
            for (int i = 0; i < bitsPerAxis; i++) {
                code |= (((quantized.x) >> i) & 0x1u) << (3 * i);
                code |= (((quantized.y) >> i) & 0x1u) << (3 * i + 1);
                code |= (((quantized.z) >> i) & 0x1u) << (3 * i + 2);
            }

            return code;
        }

        uint64_t calculateDelta(std::vector<std::pair<uint32_t, BVHLeaf>>& bvhMortonCodeLeaves, uint32_t i, uint32_t j)
        {
            if (bvhMortonCodeLeaves[i].first == bvhMortonCodeLeaves[j].first) {
                return 32 + std::countl_zero(i ^ j);
            }
            return std::countl_zero(bvhMortonCodeLeaves[i].first ^ bvhMortonCodeLeaves[j].first);
        }

        std::vector<BVHNode> buildBVHUsingSimpleMorton(Resources::MeshResource& mesh, glm::vec4 meshAABBLower, glm::vec4 meshAABBUpper)
        {
            std::vector<BVHNode> bvhNodes{};

            std::vector<std::pair<uint32_t, BVHNode>> bvhLeaves{};

            auto& meshIndices  = mesh.GetIndices();
            auto& meshVertices = mesh.GetVertices();
            for (size_t i = 0; i < meshIndices.size(); i += 3) {
                auto idx0 = meshIndices[i].idx;
                auto idx1 = meshIndices[i + 1].idx;
                auto idx2 = meshIndices[i + 2].idx;

                glm::vec3 v0 = meshVertices[idx0].position;
                glm::vec3 v1 = meshVertices[idx1].position;
                glm::vec3 v2 = meshVertices[idx2].position;

                uint32_t mortonCode = convertTriangleToMortonCode(v0, v1, v2, meshAABBLower, meshAABBUpper);

                auto triangleId = i / 3;

                BVHLeaf leaf{};
                leaf.triangleId = triangleId;
                leaf.v0         = v0;
                leaf.v1         = v1;
                leaf.v2         = v2;

                glm::vec4 lower = glm::vec4(glm::min(v0, glm::min(v1, v2)), 1.0);
                glm::vec4 upper = glm::vec4(glm::max(v0, glm::max(v1, v2)), 1.0);

                BVHNode node{};
                node.leaves.push_back(std::move(leaf));
                node.aabb.lower = std::move(lower);
                node.aabb.upper = std::move(upper);
                node.children   = {};

                bvhLeaves.push_back(std::make_pair(mortonCode, std::move(node)));
            }

            std::sort(bvhLeaves.begin(), bvhLeaves.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

            // Now we can create our BVH - leaves are sorted by Morton codes.
            bvhNodes.reserve(bvhLeaves.size()); // This could be improved.

            std::vector<size_t> currentLevel;
            currentLevel.reserve(bvhNodes.size());
            for (const auto& [mortonCode, node] : bvhLeaves) {
                currentLevel.push_back(bvhNodes.size());
                bvhNodes.push_back(node);
            }

            while (currentLevel.size() > 1) {
                std::vector<size_t> nextLevel;

                while (!currentLevel.empty()) {
                    auto numChildren = std::min(currentLevel.size(), static_cast<size_t>(MAX_BVH_CHILDREN));

                    auto parentIndex = bvhNodes.size();

                    BVHNode parentNode{};
                    parentNode.aabb.lower = glm::vec4(std::numeric_limits<float>::max());
                    parentNode.aabb.upper = glm::vec4(std::numeric_limits<float>::lowest());

                    parentNode.numChildren = numChildren;
                    parentNode.children.resize(numChildren);

                    for (size_t i = 0; i < numChildren; i++) {
                        auto childIndex = currentLevel.back();
                        currentLevel.pop_back();

                        auto& child           = bvhNodes[childIndex];
                        parentNode.aabb.lower = glm::min(parentNode.aabb.lower, child.aabb.lower);
                        parentNode.aabb.upper = glm::max(parentNode.aabb.upper, child.aabb.upper);

                        parentNode.children[i] = parentIndex - childIndex;
                    }
                    bvhNodes.push_back(std::move(parentNode));
                    nextLevel.push_back(parentIndex);
                }

                currentLevel = std::move(nextLevel);
            }

            std::reverse(bvhNodes.begin(), bvhNodes.end());

            return bvhNodes;
        }

        std::vector<BVHNode> buildBVHUsingKarrasMorton(Resources::MeshResource& mesh, glm::vec4 meshAABBLower, glm::vec4 meshAABBUpper)
        {
            std::vector<BVHNode> bvhNodes;

            // Create morton codes from leaves. Sort them.
            std::vector<std::pair<uint32_t, BVHLeaf>> bvhMortonCodeLeaves{};

            auto& meshIndices  = mesh.GetIndices();
            auto& meshVertices = mesh.GetVertices();

            /*
                Create Morton codes from triangle centroids. Sort them.
            */

            for (size_t i = 0; i < meshIndices.size(); i += 3) {
                auto idx0 = meshIndices[i].idx;
                auto idx1 = meshIndices[i + 1].idx;
                auto idx2 = meshIndices[i + 2].idx;

                glm::vec3 v0 = meshVertices[idx0].position;
                glm::vec3 v1 = meshVertices[idx1].position;
                glm::vec3 v2 = meshVertices[idx2].position;

                uint32_t mortonCode = convertTriangleToMortonCode(v0, v1, v2, meshAABBLower, meshAABBUpper);

                auto triangleId = i / 3;

                BVHLeaf leaf{};
                leaf.triangleId = triangleId;
                leaf.v0         = v0;
                leaf.v1         = v1;
                leaf.v2         = v2;

                bvhMortonCodeLeaves.push_back(std::make_pair(mortonCode, std::move(leaf)));
            }

            std::sort(bvhMortonCodeLeaves.begin(), bvhMortonCodeLeaves.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

            /*
                Kerras loop.
            */

            uint32_t primitiveCount    = mesh.GetIndices().size() / 3;
            uint32_t internalNodeCount = primitiveCount - 1;

            bvhNodes.resize(internalNodeCount);

            std::vector<int>             nodesContainingLeaves{};
            std::unordered_map<int, int> childToParentMap{};

            // Could be parallelized.
            // Root could be specially handled.
            for (int i = 0; i < internalNodeCount; i++) {
                uint64_t deltaLeft  = 0;
                uint64_t deltaRight = 0;

                if (i > 0) {
                    deltaLeft = calculateDelta(bvhMortonCodeLeaves, i, i - 1);
                }
                // We don't need to check delta right - we are iterating until n - 1.
                deltaRight = calculateDelta(bvhMortonCodeLeaves, i, i + 1);

                int direction = deltaLeft > deltaRight ? -1 : 1;

                uint64_t deltaMin = deltaLeft > deltaRight ? deltaRight : deltaLeft;

                int  move       = 1;
                int  iterator   = i + (direction * move);
                bool foundSplit = false;
                while (iterator >= 0 && iterator <= internalNodeCount) {
                    uint64_t delta = calculateDelta(bvhMortonCodeLeaves, i, iterator);

                    if (delta <= deltaMin) {
                        move--; // go back by one
                        foundSplit = true;
                    } else {
                        move++;
                    }

                    iterator = i + (direction * move);

                    if (delta <= deltaMin) {
                        break;
                    }
                }

                // It needs to cover the whole range - special case for root.
                if (!foundSplit) {
                    iterator = i + (direction * (move - 1));
                }

                // It is possible that a node will contain leaf and internal node as children.
                // If any of the side count is equal to 1, then it will be selected as a leaf. In any other case it's an internal node.
                int rangeBegin = std::min(i, iterator);
                int rangeEnd   = std::max(iterator, i);

                uint64_t deltaNode = calculateDelta(bvhMortonCodeLeaves, rangeBegin, rangeEnd);

                // Should be binary search.
                foundSplit = false;
                move       = 1;
                iterator   = rangeBegin;
                while (iterator >= rangeBegin && iterator <= rangeEnd) {
                    uint64_t delta = calculateDelta(bvhMortonCodeLeaves, rangeBegin, iterator);

                    if (delta <= deltaNode) {
                        // This is the split point.
                        iterator--;
                        break;
                    } else {
                        iterator++;
                    }
                }

                BVHNode& node = bvhNodes[i];

                int splitPoint = iterator; // because we went from the left side.

                int              leftCount  = splitPoint - rangeBegin + 1;
                int              rightCount = rangeEnd - splitPoint;
                std::vector<int> counts{leftCount, rightCount};

                for (int offset = 0; offset <= 1; offset++) {
                    if (counts[offset] == 1) {
                        nodesContainingLeaves.push_back(i);

                        node.leaves.emplace_back(bvhMortonCodeLeaves[splitPoint + offset].second);

                        glm::vec4 v0 = glm::vec4(bvhMortonCodeLeaves[splitPoint + offset].second.v0, 1.0f);
                        glm::vec4 v1 = glm::vec4(bvhMortonCodeLeaves[splitPoint + offset].second.v1, 1.0f);
                        glm::vec4 v2 = glm::vec4(bvhMortonCodeLeaves[splitPoint + offset].second.v2, 1.0f);

                        AABB triAABB{};
                        triAABB.lower = glm::min(glm::min(v0, v1), v2);
                        triAABB.upper = glm::max(glm::max(v0, v1), v2);

                        node.aabb.lower = glm::min(node.aabb.lower, triAABB.lower);
                        node.aabb.upper = glm::max(node.aabb.upper, triAABB.upper);
                    } else {
                        node.children.push_back(splitPoint + offset - i);
                        node.numChildren++;
                        childToParentMap[splitPoint + offset] = i;
                    }
                }
            }

            /*
                AABB Propagation - from nodes containing leaves up to the root.
            */
            for (const auto& leafId : nodesContainingLeaves) {
                AABB& aabbToPropagate = bvhNodes[leafId].aabb;

                auto parent = leafId;

                while (childToParentMap.find(parent) != childToParentMap.end()) {
                    bvhNodes[parent].aabb.lower = glm::min(bvhNodes[parent].aabb.lower, aabbToPropagate.lower);
                    bvhNodes[parent].aabb.upper = glm::max(bvhNodes[parent].aabb.upper, aabbToPropagate.upper);

                    parent = childToParentMap[parent];
                }

                bvhNodes[parent].aabb.lower = glm::min(bvhNodes[parent].aabb.lower, aabbToPropagate.lower);
                bvhNodes[parent].aabb.upper = glm::max(bvhNodes[parent].aabb.upper, aabbToPropagate.upper);
            }

            return bvhNodes;
        }
    } // namespace

    Loaders::CPUBVHBuildMortonLoader::result_type
    CPUBVHBuildMortonLoader::operator()(Resources::MeshResource& mesh, glm::vec4 meshAABBLower, glm::vec4 meshAABBUpper) const
    {
        // TODO: Do it by setting.

        // auto bvhNodes = buildBVHUsingSimpleMorton(mesh, meshAABBLower, meshAABBUpper);

        auto bvhNodes = buildBVHUsingKarrasMorton(mesh, meshAABBLower, meshAABBUpper);

        Resources::CPUBVHResource bvhResource(std::move(bvhNodes));
        return bvhResource;
    }
} // namespace Prism::Loaders