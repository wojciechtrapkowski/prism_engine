#pragma once

#include "resources/vulkan_resource.hpp"

#include "resources/window_resource.hpp"

namespace Prism::Loaders {
    struct VulkanLoader {
        using result_type = std::optional<Resources::VulkanResource>;

        VulkanLoader() = default;
        ~VulkanLoader() = default;

        VulkanLoader(VulkanLoader &&other) = default;
        VulkanLoader &operator=(VulkanLoader &&) = default;

        VulkanLoader(VulkanLoader &other) = delete;
        VulkanLoader &operator=(VulkanLoader &) = delete;

        result_type operator()(Resources::WindowResource &windowResource);
    };
} // namespace Prism::Loaders