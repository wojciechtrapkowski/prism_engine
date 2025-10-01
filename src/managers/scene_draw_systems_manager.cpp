#include "managers/scene_draw_systems_manager.hpp"

namespace Prism::Managers {
    namespace {
        std::vector<VkSemaphore> createSemaphores(VkDevice device, size_t count) {
            std::vector<VkSemaphore> semaphores(count);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            for (auto &sem : semaphores) {
                if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &sem) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create semaphore!");
                }
            }

            return semaphores;
        }

        std::vector<Resources::VkCommandPoolResource> createCommandPools(VkDevice device, uint32_t graphicsQueueFamilyIndex, size_t count) {
            std::vector<Resources::VkCommandPoolResource> pools;
            pools.reserve(count);

            for (size_t i = 0; i < count; ++i) {
                pools.emplace_back(device, graphicsQueueFamilyIndex);
            }

            return pools;
        }

    } // namespace

    SceneDrawSystemsManager::SceneDrawSystemsManager(Resources::ContextResources &contextResources)
        : m_contextResources(contextResources), screenClearingSystem{contextResources}, meshDrawingSystem{contextResources}, uiDrawingSystem{contextResources},
          presentSystem{contextResources}, gizmoDrawingSystem{contextResources} {
        auto &vulkanResource = m_contextResources.GetVulkanResource();
        auto device = vulkanResource.GetDevice();
        auto graphicsQueueFamilyIndex = vulkanResource.GetGraphicsQueueFamilyIndex();
        auto framesInFlight = vulkanResource.GetFramesInFlight();

        m_commandPools = createCommandPools(device, graphicsQueueFamilyIndex, framesInFlight);

        m_updateSemaphores = createSemaphores(device, framesInFlight);
        m_renderSemaphores = createSemaphores(device, framesInFlight);
    }

    SceneDrawSystemsManager::~SceneDrawSystemsManager() {
        auto &vulkanResource = m_contextResources.GetVulkanResource();

        for (auto &sem : m_updateSemaphores) {
            vkDestroySemaphore(vulkanResource.GetDevice(), sem, nullptr);
        }
        for (auto &sem : m_renderSemaphores) {
            vkDestroySemaphore(vulkanResource.GetDevice(), sem, nullptr);
        }
    }

    void SceneDrawSystemsManager::Initialize() {
        screenClearingSystem.Initialize();
        meshDrawingSystem.Initialize();
        gizmoDrawingSystem.Initialize();
        uiDrawingSystem.Initialize();
        presentSystem.Initialize();
    }

    void SceneDrawSystemsManager::Update(float deltaTime, Resources::Scene &scene, Resources::VkStagingBufferResource &stagingBuffer) {
        auto &vulkanResource = m_contextResources.GetVulkanResource();
        auto &swapchainBoundResourceStorage = vulkanResource.GetSwapchainBoundStorage();

        auto &currentCommandPoolResource = m_commandPools.at(vulkanResource.GetCurrentFrameOffset());
        auto &currentUpdateSemaphore = m_updateSemaphores.at(vulkanResource.GetCurrentFrameOffset());
        auto &currentRenderSemaphore = m_renderSemaphores.at(vulkanResource.GetCurrentFrameOffset());
        auto imageAcquiredSemaphore = vulkanResource.GetCurrentImageAcquiredSemaphore();
        auto currentFence = vulkanResource.GetCurrentFence();

        // This could be probably moved to frame swap system.
        vulkanResource.AdvanceFrame();

        auto renderTargetOpt =
            swapchainBoundResourceStorage.Get<Resources::RenderTargetResource>(RENDER_TARGET_RESOURCE_ID, vulkanResource.GetCurrentImageIndex());
        if (!renderTargetOpt) {
            uint32_t flags = 0;
            flags |= Resources::RenderTargetResource::RenderTargetCreationFlags::COLOR_ATTACHMENT;
            flags |= Resources::RenderTargetResource::RenderTargetCreationFlags::DEPTH_STENCIL_ATTACHMENT;

            auto renderTarget = std::make_unique<Resources::RenderTargetResource>(vulkanResource.GetDevice(), vulkanResource.GetVmaAllocator(),
                                                                                  vulkanResource.GetSwapchainExtent(), flags);

            swapchainBoundResourceStorage.Insert<Resources::RenderTargetResource>(RENDER_TARGET_RESOURCE_ID, std::move(renderTarget),
                                                                                  vulkanResource.GetCurrentImageIndex());
            renderTargetOpt =
                swapchainBoundResourceStorage.Get<Resources::RenderTargetResource>(RENDER_TARGET_RESOURCE_ID, vulkanResource.GetCurrentImageIndex());
        }
        auto &renderTarget = renderTargetOpt->get();

        currentCommandPoolResource.Reset();

        { // Update
            auto commandBuffersScope = currentCommandPoolResource.BeginScope();

            screenClearingSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene);
            meshDrawingSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene);
            uiDrawingSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene);
            gizmoDrawingSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene);
            presentSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene);

            stagingBuffer.Commit(commandBuffersScope.GetNextCommandBuffer());

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = 0;
            submitInfo.pWaitSemaphores = nullptr;
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffersScope.size());
            submitInfo.pCommandBuffers = commandBuffersScope.data();
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &currentUpdateSemaphore;

            vkQueueSubmit(vulkanResource.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        }

        { // Render
            auto commandBuffersScope = currentCommandPoolResource.BeginScope();

            screenClearingSystem.Render(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene, renderTarget);
            meshDrawingSystem.Render(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene, renderTarget);
            uiDrawingSystem.Render(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene, renderTarget);
            gizmoDrawingSystem.Render(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene, renderTarget);
            presentSystem.Render(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene, renderTarget);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkSemaphore waitSemaphores[] = {currentUpdateSemaphore, imageAcquiredSemaphore};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 2;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffersScope.size());
            submitInfo.pCommandBuffers = commandBuffersScope.data();
            submitInfo.signalSemaphoreCount = 1;
            VkSemaphore signalSemaphores[] = {currentRenderSemaphore};
            submitInfo.pSignalSemaphores = signalSemaphores;

            vkQueueSubmit(vulkanResource.GetGraphicsQueue(), 1, &submitInfo, currentFence);
        }

        { // Present
            auto currentImageIndex = vulkanResource.GetCurrentImageIndex();

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &currentRenderSemaphore;
            VkSwapchainKHR swapchains[] = {vulkanResource.GetSwapchain()};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapchains;
            presentInfo.pImageIndices = &currentImageIndex;

            vkQueuePresentKHR(vulkanResource.GetPresentationQueue(), &presentInfo);
        }
    }
} // namespace Prism::Managers