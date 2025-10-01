#pragma once

#include "resources/resource.hpp"
#include "resources/vulkan/vk_buffer_resource.hpp"

#include "vk_mem_alloc.h"
#include "vulkan/vulkan.h"

#include <vector>

namespace Prism::Resources {
    struct PendingCopy {
        VkBuffer destination;
        VkBufferCopy region;
    };

    struct VkStagingBufferResource : ResourceImpl<VkStagingBufferResource> {
        VkStagingBufferResource(VmaAllocator allocator);
        ~VkStagingBufferResource() = default;

        VkStagingBufferResource(VkStagingBufferResource &&other) noexcept;

        VkStagingBufferResource &operator=(VkStagingBufferResource &&other) noexcept;

        VkStagingBufferResource(const VkStagingBufferResource &) = delete;
        VkStagingBufferResource &operator=(const VkStagingBufferResource &) = delete;

        void Copy(VkBuffer destination, void *data, size_t size);

        void Commit(VkCommandBuffer commandBuffer);

      private:
        static constexpr const VkDeviceSize INITIAL_SIZE = 10000;
        friend void swap(VkStagingBufferResource &first, VkStagingBufferResource &second) noexcept;

        VmaAllocator allocator = VK_NULL_HANDLE;

        Resources::VkBufferResource<> stagingBuffer = {};
        size_t currentlyUtilized = 0;


        std::vector<PendingCopy> pendingCopies = {};
    };
} // namespace Prism::Resources