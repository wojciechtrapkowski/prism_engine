#include "resources/vulkan_resource.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "utils/vulkan/common.hpp"

#include <set>
#include <utility>

namespace Prism::Resources {
    namespace {
        struct SwapchainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        SwapchainSupportDetails querySwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice device) {
            SwapchainSupportDetails details;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

            if (formatCount != 0) {
                details.formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

            if (presentModeCount != 0) {
                details.presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
            }

            return details;
        }

        std::vector<VkFence> createFences(VkDevice device) {
            std::vector<VkFence> fences;

            for (size_t i = 0; i < Resources::VulkanResource::FRAMES_IN_FLIGHT; i++) {
                VkFence fence = VK_NULL_HANDLE;

                VkFenceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT};

                if (vkCreateFence(device, &createInfo, nullptr, &fence) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create fence!");
                }

                fences.push_back(fence);
            }

            return fences;
        }

        std::vector<VkSemaphore> createSemaphores(VkDevice device) {
            std::vector<VkSemaphore> semaphores;
            semaphores.reserve(Resources::VulkanResource::FRAMES_IN_FLIGHT);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreInfo.pNext = nullptr;
            semaphoreInfo.flags = 0;

            for (uint32_t i = 0; i < Resources::VulkanResource::FRAMES_IN_FLIGHT; ++i) {
                VkSemaphore semaphore = VK_NULL_HANDLE;
                if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
                    // Cleanup any semaphores that succeeded before throwing.
                    for (VkSemaphore s : semaphores) {
                        if (s != VK_NULL_HANDLE)
                            vkDestroySemaphore(device, s, nullptr);
                    }
                    throw std::runtime_error("Failed to create semaphore!");
                }
                semaphores.push_back(semaphore);
            }

            return semaphores;
        }

    } // namespace

    VulkanResource::VulkanResource(VkInstance instance, std::unique_ptr<Utils::Vulkan::DebugMessenger> debugMessenger, VkSurfaceKHR surface,
                                   VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkQueue presentationQueue,
                                   VkExtent2D swapchainExtent)
        : instance(instance), debugMessenger(std::move(debugMessenger)), surface(surface), physicalDevice(physicalDevice), device(device),
          vmaAllocator(allocator), graphicsQueue(graphicsQueue), presentationQueue(presentationQueue), fences(createFences(device)),
          imageAcquiredSemaphores(createSemaphores(device)) {

        RecreateSwapchain(swapchainExtent.width, swapchainExtent.height);
    }

    VulkanResource::~VulkanResource() {
        // This is global, we don't need to wait on device idle.

        // Explicit deletion.
        debugMessenger = nullptr;

        if (device != VK_NULL_HANDLE) {
            for (auto semaphore : imageAcquiredSemaphores) {
                vkDestroySemaphore(device, semaphore, nullptr);
            }

            cleanupSwapchain();

            for (auto fence : fences) {
                vkDestroyFence(device, fence, nullptr);
            }

            vmaDestroyAllocator(vmaAllocator);

            vkDestroyDevice(device, nullptr);
        }
        if (surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }
        if (instance != VK_NULL_HANDLE) {
            vkDestroyInstance(instance, nullptr);
        }
    }

    VulkanResource::VulkanResource(VulkanResource &&other) noexcept { swap(*this, other); }

    VulkanResource &VulkanResource::operator=(VulkanResource &&other) noexcept {
        if (this != &other) {
            swap(*this, other);
        }
        return *this;
    }

    void swap(VulkanResource &first, VulkanResource &second) noexcept {
        using std::swap;

        swap(first.instance, second.instance);
        swap(first.surface, second.surface);
        swap(first.physicalDevice, second.physicalDevice);
        swap(first.device, second.device);
        swap(first.vmaAllocator, second.vmaAllocator);
        swap(first.graphicsQueue, second.graphicsQueue);
        swap(first.presentationQueue, second.presentationQueue);

        swap(first.graphicsQueueFamilyIndex, second.graphicsQueueFamilyIndex);
        swap(first.presentationQueueFamilyIndex, second.presentationQueueFamilyIndex);

        swap(first.swapchainImageFormat, second.swapchainImageFormat);
        swap(first.swapchainExtent, second.swapchainExtent);
        swap(first.swapchain, second.swapchain);

        swap(first.swapchainImages, second.swapchainImages);
        swap(first.swapchainImagesViews, second.swapchainImagesViews);
        swap(first.fences, second.fences);
        swap(first.imageAcquiredSemaphores, second.imageAcquiredSemaphores);

        swap(first.imageCount, second.imageCount);
        swap(first.currentImageIndex, second.currentImageIndex);
        swap(first.currentFrameOffset, second.currentFrameOffset);

        swap(first.debugMessenger, second.debugMessenger);
    }

    void VulkanResource::RecreateSwapchain(int newWidth, int newHeight) {
        // Wait until everything is finished to avoid synchronisation issues.
        vkDeviceWaitIdle(device);

        cleanupSwapchain();
        createSwapchain(newWidth, newHeight);
        createSwapchainImages();
        createSwapchainImagesViews();
    }

    void VulkanResource::AdvanceFrame() {
        vkWaitForFences(device, 1, &fences[currentFrameOffset], true, UINT64_MAX);
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquiredSemaphores[currentFrameOffset], VK_NULL_HANDLE, &currentImageIndex);
        vkResetFences(device, 1, &fences[currentFrameOffset]);

        currentFrameOffset = (currentFrameOffset + 1) % FRAMES_IN_FLIGHT;
    }

    void VulkanResource::cleanupSwapchain() {
        swapchainBoundResourceStorage.Clear();

        for (auto imageView : swapchainImagesViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        swapchainImagesViews.clear();

        swapchainImages.clear();

        if (swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            swapchain = VK_NULL_HANDLE;
        }
    }

    void VulkanResource::createSwapchain(int newWidth, int newHeight) {
        auto swapchainSupport = querySwapChainSupport(surface, physicalDevice);

        auto checkFormatSupport = [this](const VkSurfaceFormatKHR &availableFormat) {
            return availableFormat.format == USED_SURFACE_FORMAT.format && availableFormat.colorSpace == USED_SURFACE_FORMAT.colorSpace;
        };
        if (std::find_if(swapchainSupport.formats.begin(), swapchainSupport.formats.end(), checkFormatSupport) == swapchainSupport.formats.end()) {
            throw std::runtime_error("Format & color space are not supported on this device!");
        }

        auto checkPresentSupport = [this](const VkPresentModeKHR &availablePresentMode) { return availablePresentMode == PRESENT_MODE; };
        if (std::find_if(swapchainSupport.presentModes.begin(), swapchainSupport.presentModes.end(), checkPresentSupport) ==
            swapchainSupport.presentModes.end()) {
            throw std::runtime_error("Present mode is not supported on this device!");
        }

        VkExtent2D extent = {static_cast<uint32_t>(newWidth), static_cast<uint32_t>(newHeight)};

        extent.width = std::clamp(extent.width, swapchainSupport.capabilities.minImageExtent.width, swapchainSupport.capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, swapchainSupport.capabilities.minImageExtent.height, swapchainSupport.capabilities.maxImageExtent.height);

        // We want 2 swapchain images.
        imageCount = std::max(2u, swapchainSupport.capabilities.minImageCount);

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = USED_SURFACE_FORMAT.format;
        createInfo.imageColorSpace = USED_SURFACE_FORMAT.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        Utils::Vulkan::Common::QueueFamilyIndices indices = Utils::Vulkan::Common::findQueueFamilies(surface, physicalDevice);

        graphicsQueueFamilyIndex = indices.graphicsFamily.value();
        presentationQueueFamilyIndex = indices.presentFamily.value();

        std::set<uint32_t> queueFamilySet = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        std::vector<uint32_t> queueFamilyIndices(queueFamilySet.begin(), queueFamilySet.end());

        createInfo.imageSharingMode = queueFamilyIndices.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();

        createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = PRESENT_MODE;
        createInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        swapchainExtent = {static_cast<uint32_t>(newWidth), static_cast<uint32_t>(newHeight)};
    }

    void VulkanResource::createSwapchainImages() {
        uint32_t imageCount;
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);

        swapchainImages.resize(imageCount);

        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
    }

    void VulkanResource::createSwapchainImagesViews() {
        swapchainImagesViews.resize(swapchainImages.size());

        for (size_t i = 0; i < swapchainImagesViews.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = USED_SURFACE_FORMAT.format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImagesViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }
} // namespace Prism::Resources