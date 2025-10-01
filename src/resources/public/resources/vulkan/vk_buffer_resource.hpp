#pragma once

#include "resources/resource.hpp"

#include "vk_mem_alloc.h"
#include "vulkan/vulkan.h"

#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace Prism::Resources {
    template <typename T = void> struct VkBufferResource : ResourceImpl<VkBufferResource<T>> {
      private:
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceSize bufferSize = 0;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocator allocator = VK_NULL_HANDLE;

      public:
        VkBufferResource() = default;

        explicit VkBufferResource(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO)
            : buffer(VK_NULL_HANDLE), bufferSize(size), allocation(VK_NULL_HANDLE), allocator(allocator) {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = memoryUsage;

            if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
                buffer = VK_NULL_HANDLE;
                allocation = VK_NULL_HANDLE;
                throw std::runtime_error("Failed to create Vulkan buffer!");
            }
        }

        ~VkBufferResource() {
            if (buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE && allocator != VK_NULL_HANDLE) {
                vmaDestroyBuffer(allocator, buffer, allocation);
            }
        }

        friend void swap(VkBufferResource<T> &first, VkBufferResource<T> &second) noexcept {
            using std::swap;
            swap(first.buffer, second.buffer);
            swap(first.bufferSize, second.bufferSize);
            swap(first.allocation, second.allocation);
            swap(first.allocator, second.allocator);
        }

        VkBufferResource(VkBufferResource &&other) noexcept { swap(*this, other); }

        VkBufferResource &operator=(VkBufferResource &&other) noexcept {
            if (this != &other) {
                swap(*this, other);
            }
            return *this;
        }

        VkBufferResource(const VkBufferResource &) = delete;
        VkBufferResource &operator=(const VkBufferResource &) = delete;

        VkBuffer GetBuffer() const { return buffer; }

        VmaAllocation GetAllocation() const { return allocation; }

        VkDeviceSize GetBufferSize() const { return bufferSize; }

        constexpr VkDeviceSize GetElementSize() const
            requires(!std::is_void_v<T>)
        {
            return static_cast<VkDeviceSize>(sizeof(T));
        }

        VkDeviceSize GetElementCount() const
            requires(!std::is_void_v<T>)
        {
            return bufferSize / static_cast<VkDeviceSize>(sizeof(T));
        }
    };
} // namespace Prism::Resources