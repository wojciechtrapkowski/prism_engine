#include "loaders/vulkan_loader.hpp"

#include "utils/vulkan/common.hpp"
#include "utils/vulkan/debug_messenger.hpp"

#include "GLFW/glfw3.h"
#include "vk_mem_alloc.h"

#include <algorithm>
#include <array>
#include <format>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>

namespace Prism::Loaders {
    namespace {
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

        std::vector<const char *> getRequiredDeviceExtensions() {
            std::vector<const char *> deviceExtensions;
            deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef PLATFORM_MAC
            deviceExtensions.push_back("VK_KHR_portability_subset");
#endif

            return deviceExtensions;
        }

        std::vector<const char *> getValidationLayers() {
            std::vector<const char *> validationLayers;
#ifdef DEBUG
            validationLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

            return validationLayers;
        }

        bool checkPhysicalDeviceExtensionsSupport(VkPhysicalDevice device) {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            auto deviceExtensions = getRequiredDeviceExtensions();
            std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

            for (const auto &extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
            }

            return requiredExtensions.empty();
        }

        VkInstance createInstance() {
            VkResult result;

            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Prism UI";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "Prism";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_3;

            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            auto extensions = getRequiredInstanceExtensions();

            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
            createInfo.flags = 0;

#ifdef PLATFORM_MAC
            createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

            auto validationLayers = getValidationLayers();
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

#ifdef DEBUG
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = Utils::Vulkan::DebugMessenger::getCreateInfo();
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
#endif

            VkInstance instance;
            result = vkCreateInstance(&createInfo, nullptr, &instance);
            if (result != VK_SUCCESS) {
                throw std::runtime_error(std::format("Failed to create Vulkan instance! Error code: {}\n", static_cast<int>(result)).c_str());
            }

            return instance;
        }

        VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow *window) {
            VkSurfaceKHR surface;

            if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create window surface!");
            }

            return surface;
        }

        VkPhysicalDevice pickPhysicalDevice(VkInstance instance) {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

            if (deviceCount == 0) {
                throw std::runtime_error("Couldn't find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());


            for (const auto &device : devices) {
                if (checkPhysicalDeviceExtensionsSupport(device)) {
                    return device;
                }
            }

            throw std::runtime_error("Couldn't find a suitable GPU!");
        };

        VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, Utils::Vulkan::Common::QueueFamilyIndices indices) {
            VkDevice device;

            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;

                queueCreateInfos.push_back(queueCreateInfo);
            }

            VkPhysicalDeviceFeatures deviceFeatures{};

            VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
            dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
            dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos = queueCreateInfos.data();

            createInfo.pEnabledFeatures = &deviceFeatures;

            std::vector<const char *> deviceExtensions = getRequiredDeviceExtensions();
            createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
            createInfo.ppEnabledExtensionNames = deviceExtensions.data();

            auto validationLayers = getValidationLayers();

            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            createInfo.pNext = &dynamicRenderingFeatures;

            if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create logical device!");
            }

            return device;
        }

        std::pair<VkQueue, VkQueue> getQueues(VkDevice device, Utils::Vulkan::Common::QueueFamilyIndices indices) {

            VkQueue graphicsQueue;
            vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);

            VkQueue presentationQueue;
            vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentationQueue);

            return std::make_pair(graphicsQueue, presentationQueue);
        }
    }; // namespace

    VulkanLoader::result_type VulkanLoader::operator()(Resources::WindowResource &windowResource) {
        try {
            auto instance = createInstance();
#ifdef DEBUG
            auto debugMessenger = std::make_unique<Utils::Vulkan::DebugMessenger>(instance);
#else
            auto debugMessenger = nullptr;
#endif

            auto surface = createSurface(instance, windowResource.GetWindow());

            auto physicalDevice = pickPhysicalDevice(instance);

            Utils::Vulkan::Common::QueueFamilyIndices indices = Utils::Vulkan::Common::findQueueFamilies(surface, physicalDevice);

            auto device = createLogicalDevice(physicalDevice, indices);

            auto [graphicsQueue, presentationQueue] = getQueues(device, indices);

            auto [windowWidth, windowHeight] = windowResource.GetWindowExtent();
            VkExtent2D windowExtent = {.width = static_cast<uint32_t>(windowWidth), .height = static_cast<uint32_t>(windowHeight)};

            std::cout << "Vulkan loaded successfully!" << std::endl;

            VmaAllocator allocator;

            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = physicalDevice;
            allocatorInfo.device = device;
            allocatorInfo.instance = instance;

            vmaCreateAllocator(&allocatorInfo, &allocator);

            return Resources::VulkanResource(instance, std::move(debugMessenger), surface, physicalDevice, device, allocator, graphicsQueue, presentationQueue,
                                             windowExtent);

        } catch (const std::exception &e) {
            std::cerr << "Couldn't load Vulkan - " << e.what() << std::endl;

            return std::nullopt;
        }
    }
}; // namespace Prism::Loaders