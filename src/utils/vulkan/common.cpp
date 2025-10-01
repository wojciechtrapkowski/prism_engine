#include "utils/vulkan/common.hpp"

#include <fstream>

namespace Prism::Utils::Vulkan::Common {
    namespace {
        std::vector<char> readFile(const char *path) {
            std::ifstream file(path, std::ios::ate | std::ios::binary);
            if (!file.good()) {
                throw std::runtime_error(std::string("Failed to open file: ") + path);
            }

            const size_t size = (size_t)file.tellg();
            std::vector<char> buffer(size);

            file.seekg(0);
            file.read(buffer.data(), size);

            return buffer;
        }
    }; // namespace

    QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    VkShaderModule loadShaderModule(VkDevice device, const char *spvPath) {
        auto code = readFile(spvPath);

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
        createInfo.flags = 0;

        VkShaderModule module{};

        if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
            throw std::runtime_error("Couldn't create shader module!");
        }

        return module;
    }

} // namespace Prism::Utils::Vulkan::Common