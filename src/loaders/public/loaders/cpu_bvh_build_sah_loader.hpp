#pragma once

#pragma once

#include "resources/cpu_bvh_resource.hpp"
#include "resources/mesh_resource.hpp"

#include "entt/entt.hpp"

namespace Prism::Loaders
{
    struct CPUBVHBuildSAHLoader
    {
        using result_type = std::optional<Resources::CPUBVHResource>;

        CPUBVHBuildSAHLoader()  = default;
        ~CPUBVHBuildSAHLoader() = default;

        CPUBVHBuildSAHLoader(CPUBVHBuildSAHLoader&& other)      = default;
        CPUBVHBuildSAHLoader& operator=(CPUBVHBuildSAHLoader&&) = default;

        CPUBVHBuildSAHLoader(CPUBVHBuildSAHLoader& other)      = delete;
        CPUBVHBuildSAHLoader& operator=(CPUBVHBuildSAHLoader&) = delete;

        result_type operator()(Resources::MeshResource& meshResource, glm::vec4 meshAABBLower, glm::vec4 meshAABBUpper) const;
    };
} // namespace Prism::Loaders