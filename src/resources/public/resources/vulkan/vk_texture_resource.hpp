#pragma once

#include "resources/resource.hpp"
#include "resources/vulkan/vk_image_resource.hpp"

#include "volk/volk.h"
#include "vk_mem_alloc.h"

namespace Prism::Resources
{
    struct VkTextureResource : ResourceImpl<VkTextureResource>
    {
        VkTextureResource(VkDevice device, Resources::VkImageResource imageResource, VkImageView imageView, VkSampler sampler);
        ~VkTextureResource();

        VkTextureResource(VkTextureResource&& other) noexcept;

        VkTextureResource& operator=(VkTextureResource&& other) noexcept;

        VkTextureResource(const VkTextureResource&)            = delete;
        VkTextureResource& operator=(const VkTextureResource&) = delete;

        VkImage     GetImage() const { return _imageResource.GetImage(); }
        int         GetWidth() const { return _imageResource.GetWidth(); }
        int         GetHeight() const { return _imageResource.GetHeight(); }
        VkImageView GetImageView() const { return _imageView; }
        VkSampler   GetSampler() const { return _sampler; }

    private:
        friend void swap(VkTextureResource& first, VkTextureResource& second) noexcept;

        VkDevice _device = VK_NULL_HANDLE;

        Resources::VkImageResource _imageResource = {};
        VkImageView                _imageView     = VK_NULL_HANDLE;
        VkSampler                  _sampler       = VK_NULL_HANDLE;
    };
} // namespace Prism::Resources