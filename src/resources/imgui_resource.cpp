#include "resources/imgui_resource.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <iostream>

namespace Prism::Resources {
    ImGuiResource::ImGuiResource(VkDevice device, VkDescriptorPool descriptorPool, VkRenderPass renderPass)
        : isOwned(true), device(device), descriptorPool(descriptorPool), renderPass(renderPass) {}

    ImGuiResource::~ImGuiResource() {
        if (isOwned) {
            std::cout << "Shutting down imgui!" << std::endl;

            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            if (descriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            }

            if (renderPass != VK_NULL_HANDLE) {
                vkDestroyRenderPass(device, renderPass, nullptr);
            }
        }
    }

    ImGuiResource::ImGuiResource(ImGuiResource &&other) noexcept { swap(*this, other); }

    ImGuiResource &ImGuiResource::operator=(ImGuiResource &&other) noexcept {
        if (this != &other) {
            swap(*this, other);
        }
        return *this;
    }

    void swap(ImGuiResource &first, ImGuiResource &second) noexcept {
        using std::swap;

        swap(first.isOwned, second.isOwned);
        swap(first.descriptorPool, second.descriptorPool);
        swap(first.renderPass, second.renderPass);
        swap(first.device, second.device);
    }
} // namespace Prism::Resources