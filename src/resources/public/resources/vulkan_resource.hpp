#pragma once

#include "resource.hpp"

#include "utils/vulkan/debug_messenger.hpp"

#include "vulkan/vulkan.h"

#include "resources/resource_storage.hpp"

#include "vk_mem_alloc.h"

#include <vector>

namespace Prism::Resources {
    struct VulkanResource : ResourceImpl<VulkanResource> {
        VulkanResource(VkInstance instance, std::unique_ptr<Utils::Vulkan::DebugMessenger> debugMessenger, VkSurfaceKHR surface,
                       VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkQueue presentationQueue,
                       VkExtent2D swapchainExtent);
        ~VulkanResource();

        VulkanResource(const VulkanResource &other) = delete;
        VulkanResource &operator=(const VulkanResource &other) = delete;

        VulkanResource(VulkanResource &&other) noexcept;
        VulkanResource &operator=(VulkanResource &&other) noexcept;

        void RecreateSwapchain(int newWidth, int newHeight);

        void AdvanceFrame();

        // Getters

        VkInstance GetInstance() const { return instance; }

        VkSurfaceKHR GetSurface() const { return surface; }

        VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }

        VkDevice GetDevice() const { return device; }

        VkQueue GetGraphicsQueue() const { return graphicsQueue; }

        VkQueue GetPresentationQueue() const { return presentationQueue; }

        uint32_t GetGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex; }

        uint32_t GetPresentationQueueFamilyIndex() const { return presentationQueueFamilyIndex; }

        VkFormat GetSwapchainImageFormat() const { return swapchainImageFormat; }

        VkFormat GetSwapchainDepthFormat() const { return swapchainDepthFormat; }

        VkExtent2D GetSwapchainExtent() const { return swapchainExtent; }

        VkSwapchainKHR GetSwapchain() const { return swapchain; }

        std::vector<VkImageView> GetSwapchainImagesViews() const { return swapchainImagesViews; }

        const VkImage GetRenderTargetImage() const { return swapchainImages[currentImageIndex]; }

        const VkImageView GetRenderTargetImageView() const { return swapchainImagesViews[currentImageIndex]; }

        uint32_t GetImageCount() const { return imageCount; }

        uint32_t GetCurrentImageIndex() const { return currentImageIndex; }

        uint32_t GetCurrentFrameOffset() const { return currentFrameOffset; }

        VkSemaphore GetCurrentImageAcquiredSemaphore() const { return imageAcquiredSemaphores[currentFrameOffset]; }

        VkFence GetCurrentFence() const { return fences[currentFrameOffset]; }

        auto GetFramesInFlight() const { return FRAMES_IN_FLIGHT; }

        auto &GetSwapchainBoundStorage() { return swapchainBoundResourceStorage; }

        auto &GetVmaAllocator() { return vmaAllocator; }

        static constexpr auto FRAMES_IN_FLIGHT = 2;

      private:
        VkSurfaceFormatKHR USED_SURFACE_FORMAT = {.format = VK_FORMAT_B8G8R8A8_SRGB, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        VkPresentModeKHR PRESENT_MODE = VK_PRESENT_MODE_FIFO_KHR;

        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VmaAllocator vmaAllocator = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentationQueue = VK_NULL_HANDLE;

        VkFormat swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
        VkFormat swapchainDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
        VkExtent2D swapchainExtent = {0, 0};
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;

        std::vector<VkImage> swapchainImages = {};
        std::vector<VkImageView> swapchainImagesViews = {};
        std::vector<VkFence> fences = {};
        std::vector<VkSemaphore> imageAcquiredSemaphores = {};

        uint32_t imageCount = 0;
        uint32_t currentImageIndex = 0;
        int32_t currentFrameOffset = 0;

        uint32_t graphicsQueueFamilyIndex = 0;
        uint32_t presentationQueueFamilyIndex = 0;

        Resources::ResourceStorage swapchainBoundResourceStorage = {};

        std::unique_ptr<Utils::Vulkan::DebugMessenger> debugMessenger = nullptr;

        friend void swap(VulkanResource &first, VulkanResource &second) noexcept;

        void cleanupSwapchain();

        void createSwapchain(int newWidth, int newHeight);
        void createSwapchainImages();
        void createSwapchainImagesViews();
    };

} // namespace Prism::Resources