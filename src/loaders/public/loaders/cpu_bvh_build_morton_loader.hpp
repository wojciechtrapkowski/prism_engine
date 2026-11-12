#pragma once

#include "resources/cpu_bvh_resource.hpp"
#include "resources/mesh_resource.hpp"

#include "entt/entt.hpp"

namespace Prism::Loaders
{
    struct CPUBVHBuildMortonLoader
    {
        using result_type = std::optional<Resources::CPUBVHResource>;

        CPUBVHBuildMortonLoader()  = default;
        ~CPUBVHBuildMortonLoader() = default;

        CPUBVHBuildMortonLoader(CPUBVHBuildMortonLoader&& other)      = default;
        CPUBVHBuildMortonLoader& operator=(CPUBVHBuildMortonLoader&&) = default;

        CPUBVHBuildMortonLoader(CPUBVHBuildMortonLoader& other)      = delete;
        CPUBVHBuildMortonLoader& operator=(CPUBVHBuildMortonLoader&) = delete;

        result_type operator()(Resources::MeshResource& meshResource, glm::vec4 meshAABBLower, glm::vec4 meshAABBUpper) const;
    };
} // namespace Prism::Loaders