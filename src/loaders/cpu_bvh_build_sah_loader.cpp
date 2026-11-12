#pragma once

#include "loaders/cpu_bvh_build_sah_loader.hpp"

namespace Prism::Loaders
{
    namespace
    {} // namespace

    Loaders::CPUBVHBuildSAHLoader::result_type
    CPUBVHBuildSAHLoader::operator()(Resources::MeshResource& mesh, glm::vec4 meshAABBLower, glm::vec4 meshAABBUpper) const
    {
        using BVHNode = Resources::CPUBVHResource::BVHNode;
        using BVHLeaf = Resources::CPUBVHResource::BVHLeaf;

        std::vector<BVHNode> bvhNodes{};

        Resources::CPUBVHResource bvhResource(std::move(bvhNodes));
        return bvhResource;
    }
} // namespace Prism::Loaders