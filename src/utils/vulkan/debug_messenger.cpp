#include "utils/vulkan/debug_messenger.hpp"

namespace Prism::Utils::Vulkan {
    VkDebugUtilsMessengerCreateInfoEXT DebugMessenger::getCreateInfo() {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        return createInfo;
    }

    DebugMessenger::DebugMessenger(VkInstance instance) : instance(instance) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = getCreateInfo();

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, &createInfo, nullptr, &debugMessenger);
        } else {
            throw std::runtime_error("Failed to set up debug messenger!");
        }
    }

    DebugMessenger::~DebugMessenger() {
        if (debugMessenger != VK_NULL_HANDLE) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(instance, debugMessenger, nullptr);
            }
        }
    }

    DebugMessenger::DebugMessenger(DebugMessenger &&other) noexcept { swap(*this, other); }

    DebugMessenger &DebugMessenger::operator=(DebugMessenger &&other) noexcept {
        if (this != &other) {
            swap(*this, other);
        }
        return *this;
    }

    void swap(DebugMessenger &first, DebugMessenger &second) noexcept {
        using std::swap;
        swap(first.instance, second.instance);
        swap(first.debugMessenger, second.debugMessenger);
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessenger::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                 const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }
} // namespace Prism::Utils::Vulkan
