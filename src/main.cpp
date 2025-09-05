#include "context/context.hpp"

#include <iostream>
#include <stdexcept>

#include <vulkan/vulkan.h>

int main() {
    VkResult result;

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "HelloVulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

#ifdef PLATFORM_MAC
    // macOS / MoltenVK
    const char *extensions[] = {
        "VK_KHR_surface",
        "VK_MVK_macos_surface",
        "VK_KHR_portability_enumeration"
    };
    createInfo.enabledExtensionCount = 3;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#else
    // Windows / Linux
    const char *extensions[] = {
        "VK_KHR_surface",
        // optionally "VK_KHR_win32_surface" on Windows
    };
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.flags = 0;
#endif

    VkInstance instance;
    result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance! Error code: " << result
                  << std::endl;
        return -1;
    }

    std::cout << "Vulkan instance created successfully!" << std::endl;

    vkDestroyInstance(instance, nullptr);

    try {
        Prism::Context::Context context;
        context.RunEngine();
    } catch (const std::exception &e) {
        std::cerr << "Shader compilation error: " << e.what() << std::endl;
    }

    return 0;
}
