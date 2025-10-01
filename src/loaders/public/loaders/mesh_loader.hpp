#pragma once

#include "resources/mesh_resource.hpp"
#include "resources/vulkan/vk_staging_buffer_resource.hpp"
#include "resources/vulkan_resource.hpp"

#include <optional>
#include <string>
#include <utility>

namespace Prism::Loaders {
    struct MeshLoader {
        using result_type = std::optional<std::unique_ptr<Resources::MeshResource>>;

        MeshLoader() = default;
        ~MeshLoader() = default;

        MeshLoader(MeshLoader &&other) = default;
        MeshLoader &operator=(MeshLoader &&) = default;

        MeshLoader(MeshLoader &other) = delete;
        MeshLoader &operator=(MeshLoader &) = delete;

        result_type operator()(Resources::VulkanResource &vulkanResource, Resources::VkStagingBufferResource &stagingBuffer, const std::string &path) const;
    };
}; // namespace Prism::Loaders