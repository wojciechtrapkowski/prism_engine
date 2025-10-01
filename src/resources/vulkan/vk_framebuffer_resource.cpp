#include "resources/vulkan/vk_framebuffer_resource.hpp"

namespace Prism::Resources {
    VkFramebufferResource::VkFramebufferResource(VkDevice device, VkFramebuffer fb) : device(device), framebuffer(fb) {}

    VkFramebufferResource::VkFramebufferResource(VkFramebufferResource &&other) noexcept { swap(*this, other); }

    VkFramebufferResource &VkFramebufferResource::operator=(VkFramebufferResource &&other) noexcept {
        if (this != &other) {
            swap(*this, other);
        }
        return *this;
    }

    VkFramebufferResource::~VkFramebufferResource() {
        if (framebuffer != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }

    void swap(VkFramebufferResource &first, VkFramebufferResource &second) noexcept {
        using std::swap;
        swap(first.device, second.device);
        swap(first.framebuffer, second.framebuffer);
    }
} // namespace Prism::Resources