#include "context/context.hpp"


#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

std::vector<const char *> getRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    extensions.push_back("VK_KHR_surface");

#ifdef PLATFORM_MAC
    extensions.push_back("VK_MVK_macos_surface");
    extensions.push_back("VK_KHR_portability_enumeration");
    extensions.push_back("VK_KHR_get_physical_device_properties2");
#endif

    return extensions;
};

int main() {
    try {
        Prism::Context::Context context;
        context.RunEngine();
    } catch (const std::exception &e) {
        std::cerr << "Shader compilation error: " << e.what() << std::endl;
    }
    return 0;
}
