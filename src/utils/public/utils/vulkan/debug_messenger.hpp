#pragma once

#include "vulkan/vulkan.h"
#include <iostream>
#include <stdexcept>

namespace Prism::Utils::Vulkan {
    struct DebugMessenger {
        static VkDebugUtilsMessengerCreateInfoEXT getCreateInfo();

        DebugMessenger(VkInstance instance);
        ~DebugMessenger();

        DebugMessenger(const DebugMessenger &other) = delete;
        DebugMessenger &operator=(const DebugMessenger &other) = delete;

        DebugMessenger(DebugMessenger &&other) noexcept;
        DebugMessenger &operator=(DebugMessenger &&other) noexcept;

        friend void swap(DebugMessenger &first, DebugMessenger &second) noexcept;

      private:
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);
    };
} // namespace Prism::Utils::Vulkan
