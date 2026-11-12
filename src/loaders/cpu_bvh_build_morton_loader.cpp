#pragma once

#include "loaders/cpu_bvh_build_morton_loader.hpp"

namespace Prism::Loaders
{
    namespace
    {
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
    } // namespace

    Loaders::CPUBVHBuildMortonLoader::result_type
    CPUBVHBuildMortonLoader::operator()(Resources::MeshResource& mesh, glm::vec4 meshAABBLower, glm::vec4 meshAABBUpper) const
    {
        using BVHNode = Resources::CPUBVHResource::BVHNode;
        using BVHLeaf = Resources::CPUBVHResource::BVHLeaf;

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
            node.leaf       = std::move(leaf);
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

        Resources::CPUBVHResource bvhResource(std::move(bvhNodes));
        return bvhResource;
    }
} // namespace Prism::Loaders