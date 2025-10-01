#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

namespace Prism::Utils::Vulkan::Common {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };

    QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice device);

    VkShaderModule loadShaderModule(VkDevice device, const char *spvPath);
} // namespace Prism::Utils::Vulkan::Common