#pragma once

#include "resources/resource.hpp"

#include "volk/volk.h"
#include "vk_mem_alloc.h"

namespace Prism::Resources
{
    struct VkImageResource : ResourceImpl<VkImageResource>
    {
        VkImageResource() = default;
        VkImageResource(VmaAllocator allocator, VmaAllocation allocation, VkImage image, int width, int height);
        ~VkImageResource();

        VkImageResource(VkImageResource&& other) noexcept;

        VkImageResource& operator=(VkImageResource&& other) noexcept;

        VkImageResource(const VkImageResource&)            = delete;
        VkImageResource& operator=(const VkImageResource&) = delete;

        VmaAllocation GetAllocation() const { return _allocation; }
        VkImage       GetImage() const { return _image; }
        int           GetWidth() const { return _width;  }
        int           GetHeight() const { return _height; }

    private:
        friend void swap(VkImageResource& first, VkImageResource& second) noexcept;

        VmaAllocator  _allocator  = VK_NULL_HANDLE;
        VmaAllocation _allocation = VK_NULL_HANDLE;
        VkImage       _image      = VK_NULL_HANDLE;
        int           _width      = 0;
        int           _height     = 0;
    };
} // namespace Prism::Resources