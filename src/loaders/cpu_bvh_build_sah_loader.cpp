#pragma once

#include "loaders/cpu_bvh_build_sah_loader.hpp"

#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/extended_min_max.hpp>

#include <numeric>
#include <vector>

namespace Prism::Loaders
{
    namespace
    {
        using AABB    = Resources::CPUBVHResource::AABB;
        using BVHNode = Resources::CPUBVHResource::BVHNode;
        using BVHLeaf = Resources::CPUBVHResource::BVHLeaf;

        constexpr auto NUMBER_OF_AXES = 3;
        constexpr auto NUMBER_OF_BINS = 16;

        void buildBVHRecursively(
            Resources::MeshResource& mesh,
            std::vector<BVHNode>&    bvhNodes,
            std::vector<uint32_t>&   triIndices,
            std::vector<AABB>&       aabbs,
            AABB                     currentAABB,
            uint32_t                 currentNodeIndex = 0)
        {
            std::array<std::array<float, NUMBER_OF_BINS - 1>, NUMBER_OF_AXES> sahCosts{};

            auto& meshIndices  = mesh.GetIndices();
            auto& meshVertices = mesh.GetVertices();

            // Find out the best axis and plane to split on.
            for (size_t j = 1; j < NUMBER_OF_BINS; j++) {
                std::array<std::pair<AABB, AABB>, NUMBER_OF_AXES> leftRightSideAABBPerAxis{};
                std::array<std::pair<int, int>, NUMBER_OF_AXES>   leftRightSideCountPerAxis{};

                for (const auto& primitiveId : triIndices) {
                    auto& triAABB = aabbs[primitiveId];

                    for (size_t i = 0; i < NUMBER_OF_AXES; i++) {
                        float split = currentAABB.upper[i] - ((currentAABB.upper[i] - currentAABB.lower[i]) * ((float)j / (float)NUMBER_OF_BINS));

                        if ((triAABB.lower[i] + triAABB.upper[i]) * 0.5 <= split) {
                            leftRightSideCountPerAxis[i].first++;
                            leftRightSideAABBPerAxis[i].first.lower = glm::min(leftRightSideAABBPerAxis[i].first.lower, triAABB.lower);
                            leftRightSideAABBPerAxis[i].first.upper = glm::max(leftRightSideAABBPerAxis[i].first.upper, triAABB.upper);
                        } else {
                            leftRightSideCountPerAxis[i].second++;
                            leftRightSideAABBPerAxis[i].second.lower = glm::min(leftRightSideAABBPerAxis[i].second.lower, triAABB.lower);
                            leftRightSideAABBPerAxis[i].second.upper = glm::max(leftRightSideAABBPerAxis[i].second.upper, triAABB.upper);
                        }
                    }
                }

                std::pair<bool, bool> leftRightHasAnyValue = {false, false};
                for (size_t i = 0; i < NUMBER_OF_AXES; i++) {
                    leftRightHasAnyValue.first |= leftRightSideCountPerAxis[i].first > 0;
                    leftRightHasAnyValue.second |= leftRightSideCountPerAxis[i].second > 0;
                }

                // They both need to have values.
                if (!(leftRightHasAnyValue.first && leftRightHasAnyValue.second)) {
                    // We need to create new node with these primitives.

                    for (const auto& triId : triIndices) {
                        BVHLeaf leaf;

                        leaf.triangleId = triId;

                        auto idx0 = meshIndices[triId * 3 + 0];
                        auto idx1 = meshIndices[triId * 3 + 1];
                        auto idx2 = meshIndices[triId * 3 + 2];

                        auto v0 = meshVertices[idx0.idx].position;
                        auto v1 = meshVertices[idx1.idx].position;
                        auto v2 = meshVertices[idx2.idx].position;

                        leaf.v0 = v0;
                        leaf.v1 = v1;
                        leaf.v2 = v2;

                        auto& currentNode = bvhNodes[currentNodeIndex];
                        currentNode.leaves.push_back(std::move(leaf));
                    }

                    return;
                }

                // For this split on plane i, at j, calculate SAH cost
                // For now omit constants

                auto  mainBoundingBoxD = currentAABB.upper - currentAABB.lower;
                float mainBoundingBoxArea =
                    2 * (mainBoundingBoxD.x * mainBoundingBoxD.y + mainBoundingBoxD.z * mainBoundingBoxD.y + mainBoundingBoxD.z * mainBoundingBoxD.x);

                for (int i = 0; i < NUMBER_OF_AXES; i++) {
                    auto dLeft  = leftRightSideAABBPerAxis[i].first.upper - leftRightSideAABBPerAxis[i].first.lower;
                    auto dRight = leftRightSideAABBPerAxis[i].second.upper - leftRightSideAABBPerAxis[i].second.lower;

                    float leftArea  = 2 * (dLeft.x * dLeft.y + dLeft.z * dLeft.y + dLeft.z * dLeft.x);
                    float rightArea = 2 * (dRight.x * dRight.y + dRight.z * dRight.y + dRight.z * dRight.x);

                    sahCosts[i][j - 1] = (leftArea / mainBoundingBoxArea) * leftRightSideCountPerAxis[i].first +
                                         (rightArea / mainBoundingBoxArea) * leftRightSideCountPerAxis[i].second;
                }
            }

            int   selectedAxis  = -1;
            int   selectedPlane = -1;
            float minSahCost    = std::numeric_limits<float>::max();

            for (int i = 0; i < NUMBER_OF_AXES; i++) {
                for (int j = 0; j < NUMBER_OF_BINS - 1; j++) {
                    if (sahCosts[i][j] < minSahCost) {
                        minSahCost    = sahCosts[i][j];
                        selectedAxis  = i;
                        selectedPlane = j;
                    }
                }
            }

            // Now we need to divide the primitives for the left and right side and run recursively. We could probably merge this pass with the one up.
            std::vector<uint32_t> leftIndices;
            leftIndices.reserve(triIndices.size());
            AABB leftAABB{};

            std::vector<uint32_t> rightIndices;
            rightIndices.reserve(triIndices.size());
            AABB rightAABB{};

            for (const auto& primitiveId : triIndices) {
                auto& triAABB = aabbs[primitiveId];

                float split = currentAABB.upper[selectedAxis] -
                              ((currentAABB.upper[selectedAxis] - currentAABB.lower[selectedAxis]) * ((float)(selectedPlane + 1) / (float)NUMBER_OF_BINS));

                float value = ((triAABB.lower[selectedAxis] + triAABB.upper[selectedAxis]) * 0.5);

                if (value <= split) {
                    leftIndices.push_back(primitiveId);
                    leftAABB.lower = glm::min(leftAABB.lower, triAABB.lower);
                    leftAABB.upper = glm::max(leftAABB.upper, triAABB.upper);
                } else {
                    rightIndices.push_back(primitiveId);
                    rightAABB.lower = glm::min(rightAABB.lower, triAABB.lower);
                    rightAABB.upper = glm::max(rightAABB.upper, triAABB.upper);
                }
            }

            // Node creation & termination point
            // This will result in: left left left left left right right right right tree. I don't like that. Can it be fixed?

            auto runOnSubtrees = [&](std::vector<uint32_t>& indices, AABB& aabb) {
                if (indices.empty()) {
                    return;
                }

                if (indices.size() == 1) {
                    BVHLeaf leaf;
                    auto    triId   = indices.back();
                    leaf.triangleId = triId;

                    auto idx0 = meshIndices[triId * 3 + 0];
                    auto idx1 = meshIndices[triId * 3 + 1];
                    auto idx2 = meshIndices[triId * 3 + 2];

                    auto v0 = meshVertices[idx0.idx].position;
                    auto v1 = meshVertices[idx1.idx].position;
                    auto v2 = meshVertices[idx2.idx].position;

                    leaf.v0 = v0;
                    leaf.v1 = v1;
                    leaf.v2 = v2;

                    auto& currentNode = bvhNodes[currentNodeIndex];
                    currentNode.leaves.push_back(std::move(leaf));

                    return;
                }

                BVHNode node{};
                node.aabb = aabb;

                auto childIdx = bvhNodes.size() - currentNodeIndex;
                bvhNodes.push_back(std::move(node));
                auto& currentNode = bvhNodes[currentNodeIndex];
                currentNode.children.push_back(childIdx);
                currentNode.numChildren++;

                if (indices.size() > 1) {
                    buildBVHRecursively(mesh, bvhNodes, indices, aabbs, aabb, bvhNodes.size() - 1);
                }
            };

            runOnSubtrees(leftIndices, leftAABB);
            runOnSubtrees(rightIndices, rightAABB);
        }
    } // namespace

    Loaders::CPUBVHBuildSAHLoader::result_type
    CPUBVHBuildSAHLoader::operator()(Resources::MeshResource& mesh, glm::vec4 meshAABBLower, glm::vec4 meshAABBUpper) const
    {
        std::vector<BVHNode> bvhNodes{};

        auto triCount = mesh.GetIndices().size() / 3;
        if (triCount == 0) {
            return std::nullopt;
        }

        std::vector<uint32_t> triIndices(triCount);
        std::iota(triIndices.begin(), triIndices.end(), 0);

        std::vector<AABB> aabbs;
        aabbs.reserve(triCount);

        auto& meshIndices  = mesh.GetIndices();
        auto& meshVertices = mesh.GetVertices();
        for (const auto& primitiveId : triIndices) {
            auto idx0 = meshIndices[primitiveId * 3 + 0];
            auto idx1 = meshIndices[primitiveId * 3 + 1];
            auto idx2 = meshIndices[primitiveId * 3 + 2];

            auto v0 = meshVertices[idx0.idx].position;
            auto v1 = meshVertices[idx1.idx].position;
            auto v2 = meshVertices[idx2.idx].position;

            glm::vec3 triLower = glm::min(v0, v1, v2);
            glm::vec3 triUpper = glm::max(v0, v1, v2);

            AABB triAABB{glm::vec4(triLower, 1.0f), glm::vec4(triUpper, 1.0f)};
            aabbs.push_back(triAABB);
        }

        AABB rootAABB{meshAABBLower, meshAABBUpper};

        BVHNode root{};
        root.aabb = rootAABB;
        bvhNodes.push_back(std::move(root));

        buildBVHRecursively(mesh, bvhNodes, triIndices, aabbs, rootAABB);

        Resources::CPUBVHResource bvhResource(std::move(bvhNodes));
        return bvhResource;
    }
} // namespace Prism::Loaders