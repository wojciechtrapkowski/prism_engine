#pragma once

#include "resources/resource.hpp"

#include "vulkan/vulkan.h"

#include <utility>

namespace Prism::Resources {
    struct VkFramebufferResource : ResourceImpl<VkFramebufferResource> {
      public:
        explicit VkFramebufferResource(VkDevice device, VkFramebuffer framebuffer);
        ~VkFramebufferResource();

        VkFramebufferResource(VkFramebufferResource &&other) noexcept;

        VkFramebufferResource &operator=(VkFramebufferResource &&other) noexcept;

        VkFramebufferResource(const VkFramebufferResource &) = delete;
        VkFramebufferResource &operator=(const VkFramebufferResource &) = delete;

        VkFramebuffer get() const { return framebuffer; }

        friend void swap(VkFramebufferResource &first, VkFramebufferResource &second) noexcept;

      private:
        VkDevice device = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
    };
} // namespace Prism::Resources