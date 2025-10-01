#pragma once

#include "resources/resource.hpp"

#include "vulkan/vulkan.h"

namespace Prism::Resources {
    struct ImGuiResource : ResourceImpl<ImGuiResource> {
        ImGuiResource(VkDevice device, VkDescriptorPool descriptorPool, VkRenderPass renderPass);
        ~ImGuiResource();

        ImGuiResource(ImGuiResource &other) = delete;
        ImGuiResource &operator=(ImGuiResource &other) = delete;

        ImGuiResource(ImGuiResource &&other) noexcept;
        ImGuiResource &operator=(ImGuiResource &&other) noexcept;

        bool IsOwned() const { return isOwned; }

        VkRenderPass GetRenderPass() const { return renderPass; }

      private:
        bool isOwned = false;

        VkDevice device = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;

        friend void swap(ImGuiResource &first, ImGuiResource &second) noexcept;
    };
} // namespace Prism::Resources