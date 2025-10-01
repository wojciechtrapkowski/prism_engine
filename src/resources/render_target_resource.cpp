#include "resources/render_target_resource.hpp"

#include <stdexcept>
#include <utility>

namespace Prism::Resources {
    namespace {
        struct ImageAllocation {
            VkImage image = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
        };

        ImageAllocation createImage(VmaAllocator allocator, const VkImageCreateInfo &imageInfo) {
            ImageAllocation imageAllocation{};

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

            if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &imageAllocation.image, &imageAllocation.allocation, nullptr) != VK_SUCCESS) {
                throw std::runtime_error("Couldn't create an image for render target resource!");
            }

            return imageAllocation;
        }

        VkImageView createImageView(VkDevice device, VkImage image, VkFormat viewFormat, VkImageAspectFlags aspectFlags) {
            VkImageView imageView;

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = viewFormat;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                throw std::runtime_error("Coudn't create image view in render target resource!");
            }

            return imageView;
        }
    } // namespace

    RenderTargetResource::RenderTargetResource(VkDevice device, VmaAllocator allocator, VkExtent2D extent, uint32_t flags)
        : device(device), allocator(allocator), extent(extent) {

        if (flags & RenderTargetCreationFlags::COLOR_ATTACHMENT) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = COLOR_FORMAT;
            imageInfo.extent = {extent.width, extent.height, 1};
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = NUMBER_OF_SAMPLES;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            auto imageAllocation = createImage(allocator, imageInfo);

            colorImage = imageAllocation.image;
            colorImageAllocation = imageAllocation.allocation;
            colorImageView = createImageView(device, colorImage, COLOR_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT);

            // VkAttachmentDescription desc{};
            // desc.flags = 0;
            // desc.format = COLOR_FORMAT;
            // desc.samples = samples;
            // desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            // desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            // desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            // desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            // desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            // desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // VkAttachmentReference ref{};
            // ref.attachment = 0; // callers must ensure ordering matches when assembling render pass/framebuffer
            // ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // colorAttachmentDesc = desc;
            // colorAttachmentRef = ref;
        }

        if (flags & RenderTargetCreationFlags::DEPTH_STENCIL_ATTACHMENT) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = DEPTH_FORMAT;
            imageInfo.extent = {extent.width, extent.height, 1};
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = NUMBER_OF_SAMPLES;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            auto imageAllocation = createImage(allocator, imageInfo);

            depthImage = imageAllocation.image;
            depthImageAllocation = imageAllocation.allocation;
            depthImageView = createImageView(device, depthImage, DEPTH_FORMAT, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

            // VkAttachmentDescription desc{};
            // desc.flags = 0;
            // desc.format = depthFormat;
            // desc.samples = samples;
            // desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            // desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            // desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            // desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            // desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            // desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // VkAttachmentReference ref{};
            // ref.attachment = (colorAttachmentDesc.has_value() ? 1u : 0u); // if color exists it's index 1, otherwise 0
            // ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // depthAttachmentDesc = desc;
            // depthAttachmentRef = ref;
        }
    }

    RenderTargetResource::RenderTargetResource(RenderTargetResource &&other) noexcept { swap(*this, other); }

    RenderTargetResource &RenderTargetResource::operator=(RenderTargetResource &&other) noexcept {
        swap(*this, other);
        return *this;
    }

    RenderTargetResource::~RenderTargetResource() {
        if (device != VK_NULL_HANDLE) {
            if (colorImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, colorImageView, nullptr);
            }
            if (depthImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, depthImageView, nullptr);
            }
        }

        if (allocator != VK_NULL_HANDLE) {
            if (colorImage != VK_NULL_HANDLE || colorImageAllocation != VK_NULL_HANDLE) {
                vmaDestroyImage(allocator, colorImage, colorImageAllocation);
            }
            if (depthImage != VK_NULL_HANDLE || depthImageAllocation != VK_NULL_HANDLE) {
                vmaDestroyImage(allocator, depthImage, depthImageAllocation);
            }
        }
    }

    VkImage RenderTargetResource::GetColorImage() const {
        if (colorImage == VK_NULL_HANDLE) {
            throw std::runtime_error("RenderTarget error: Color image not created.");
        }
        return colorImage;
    }

    VkImageView RenderTargetResource::GetColorImageView() const {
        if (colorImageView == VK_NULL_HANDLE) {
            throw std::runtime_error("RenderTarget error: Color image view not created.");
        }
        return colorImageView;
    }

    VkImage RenderTargetResource::GetDepthImage() const {
        if (depthImage == VK_NULL_HANDLE) {
            throw std::runtime_error("RenderTarget error: Depth image not created.");
        }
        return depthImage;
    }

    VkImageView RenderTargetResource::GetDepthImageView() const {
        if (depthImageView == VK_NULL_HANDLE) {
            throw std::runtime_error("RenderTarget error: Depth image view not created.");
        }
        return depthImageView;
    }

    void swap(RenderTargetResource &a, RenderTargetResource &b) noexcept {
        using std::swap;
        swap(a.device, b.device);
        swap(a.allocator, b.allocator);
        swap(a.extent, b.extent);

        swap(a.colorImage, b.colorImage);
        swap(a.colorImageAllocation, b.colorImageAllocation);
        swap(a.colorImageView, b.colorImageView);

        swap(a.depthImage, b.depthImage);
        swap(a.depthImageAllocation, b.depthImageAllocation);
        swap(a.depthImageView, b.depthImageView);
    }

} // namespace Prism::Resources