#include "systems/ui_drawing_system.hpp"

#include "events/move_events.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

#include "vulkan/vulkan.h"

#include "resources/vulkan/vk_framebuffer_resource.hpp"

namespace Prism::Systems {
    namespace {} // namespace

    UIDrawingSystem::UIDrawingSystem(Resources::ContextResources &contextResources)
        : m_contextResources(contextResources), m_mainDockUI{contextResources}, m_menuBarUI{contextResources}, m_sceneHierarchyUI{contextResources},
          m_cameraSettingsUI{contextResources} {}

    void UIDrawingSystem::Initialize() {}

    void UIDrawingSystem::Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene &scene) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::GetIO().WantCaptureMouse) {
            m_contextResources.GetDispatcher().clear<Events::MouseMoveEvent>();
        }

        m_mainDockUI.Update(deltaTime, scene);
        m_menuBarUI.Update(deltaTime, scene);
        m_sceneHierarchyUI.Update(deltaTime, scene);
        m_cameraSettingsUI.Update(deltaTime, scene);

        vkEndCommandBuffer(commandBuffer);
    }

    void UIDrawingSystem::Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene &scene, Resources::RenderTargetResource &renderTarget) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        // Get frame buffer
        auto &vulkanResource = m_contextResources.GetVulkanResource();
        auto &swapchainBoundStorage = vulkanResource.GetSwapchainBoundStorage();
        auto currentImageIndex = vulkanResource.GetCurrentImageIndex();

        auto renderPass = m_contextResources.GetImGuiResource().GetRenderPass();

        auto framebufferOpt = swapchainBoundStorage.Get<Resources::VkFramebufferResource>(FRAMEBUFFER_RESOURCE_ID, currentImageIndex);
        if (!framebufferOpt) {
            VkDevice device = vulkanResource.GetDevice();
            VkExtent2D extent = vulkanResource.GetSwapchainExtent();

            VkImageView attachments[] = {renderTarget.GetColorImageView()};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            VkFramebuffer framebuffer;
            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer!");
            }

            auto framebufferResource = std::make_unique<Resources::VkFramebufferResource>(device, framebuffer);

            // Insert into ResourceStorage at frame index i
            swapchainBoundStorage.Insert<Resources::VkFramebufferResource>(FRAMEBUFFER_RESOURCE_ID, std::move(framebufferResource),
                                                                           static_cast<size_t>(currentImageIndex));
            framebufferOpt = swapchainBoundStorage.Get<Resources::VkFramebufferResource>(FRAMEBUFFER_RESOURCE_ID, currentImageIndex);
        }
        Resources::VkFramebufferResource &framebuffer = framebufferOpt->get();

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffer.get();
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_contextResources.GetVulkanResource().GetSwapchainExtent();
        renderPassInfo.clearValueCount = 0;
        renderPassInfo.pClearValues = nullptr;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr);

        vkCmdEndRenderPass(commandBuffer);

        vkEndCommandBuffer(commandBuffer);
    }
} // namespace Prism::Systems