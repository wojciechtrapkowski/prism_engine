#include "resources/vulkan/vk_texture_resource.hpp"

namespace Prism::Resources
{
    VkImageResource::VkImageResource(VmaAllocator allocator, VmaAllocation allocation, VkImage image, int width, int height) :
        _allocator(allocator), _allocation(allocation), _image(image), _width(width), _height(height)
    {}

    VkImageResource::~VkImageResource()
    {
        if (_allocator == VK_NULL_HANDLE) {
            return;
        }

        if (_image != VK_NULL_HANDLE) {
            vmaDestroyImage(_allocator, _image, _allocation);
        }
    }

    VkImageResource::VkImageResource(VkImageResource&& other) noexcept
    {
        swap(*this, other);
    }

    VkImageResource& VkImageResource::operator=(VkImageResource&& other) noexcept
    {
        if (this != &other) {
            swap(*this, other);
        }
        return *this;
    }

    void swap(VkImageResource& first, VkImageResource& second) noexcept
    {
        using std::swap;

        swap(first._allocator, second._allocator);
        swap(first._allocation, second._allocation);
        swap(first._image, second._image);
        swap(first._width, second._width);
        swap(first._height, second._height);
    }
} // namespace Prism::Resources