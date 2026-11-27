#pragma once

#include "resources/cpu_bvh_resource.hpp"
#include "resources/cpu_raytrace_result_resource.hpp"

#include "entt/entt.hpp"
#include "glm/glm.hpp"

#include <optional>

namespace Prism::Loaders
{
    struct CPURaytraceResultLoader
    {
        struct CPURaytraceSettings
        {
            struct
            {
                glm::mat4 projection;
                glm::mat4 view;
                glm::vec3 position;
            } cameraRelated;

            int width;
            int height;
        };

        using result_type = std::optional<Resources::CPURaytraceResultResource>;

        CPURaytraceResultLoader()  = default;
        ~CPURaytraceResultLoader() = default;

        CPURaytraceResultLoader(CPURaytraceResultLoader&& other)      = default;
        CPURaytraceResultLoader& operator=(CPURaytraceResultLoader&&) = default;

        CPURaytraceResultLoader(CPURaytraceResultLoader& other)      = delete;
        CPURaytraceResultLoader& operator=(CPURaytraceResultLoader&) = delete;

        result_type operator()(
            CPURaytraceResultLoader::CPURaytraceSettings&                   settings,
            std::vector<std::reference_wrapper<Resources::CPUBVHResource>>& bvhsToTraverse,
            std::vector<glm::mat4>&                                         transforms) const;
    };
} // namespace Prism::Loaders