#include "managers/scene_draw_systems_manager.hpp"

// Testing purposes
#include "components/transform.hpp"
#include "components/mesh.hpp"
#include "components/name.hpp"
#include "components/tags.hpp"
#include "loaders/mesh_loader.hpp"
#include <iostream>

namespace Prism::Managers
{
    namespace
    {
        std::vector<VkSemaphore> createSemaphores(VkDevice device, size_t count)
        {
            std::vector<VkSemaphore> semaphores(count);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            for (auto& sem : semaphores) {
                if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &sem) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create semaphore!");
                }
            }

            return semaphores;
        }

        std::vector<Resources::VkCommandPoolResource> createCommandPools(VkDevice device, uint32_t graphicsQueueFamilyIndex, size_t count)
        {
            std::vector<Resources::VkCommandPoolResource> pools;
            pools.reserve(count);

            for (size_t i = 0; i < count; ++i) {
                pools.emplace_back(device, graphicsQueueFamilyIndex);
            }

            return pools;
        }

        std::vector<Resources::VkStagingBufferResource> createStagingBuffers(VmaAllocator allocator, size_t count)
        {
            std::vector<Resources::VkStagingBufferResource> buffers;
            buffers.reserve(count);

            for (size_t i = 0; i < count; ++i) {
                buffers.emplace_back(allocator);
            }

            return buffers;
        }

    } // namespace

    SceneDrawSystemsManager::SceneDrawSystemsManager(Resources::ContextResources& contextResources) :
        _contextResources(contextResources), _meshLoadingSystem{contextResources}, _aabbDrawingSystem{contextResources},
        _screenClearingSystem{contextResources}, _meshDrawingSystem{contextResources}, _uiDrawingSystem{contextResources}, _presentSystem{contextResources},
        _gizmoDrawingSystem{contextResources}
    {
        auto& vulkanResource           = _contextResources.GetVulkanResource();
        auto  device                   = vulkanResource.GetDevice();
        auto  graphicsQueueFamilyIndex = vulkanResource.GetGraphicsQueueFamilyIndex();
        auto  framesInFlight           = vulkanResource.GetFramesInFlight();

        _commandPools = createCommandPools(device, graphicsQueueFamilyIndex, framesInFlight);

        _updateSemaphores = createSemaphores(device, framesInFlight);
        _renderSemaphores = createSemaphores(device, framesInFlight);

        _stagingBuffers = createStagingBuffers(vulkanResource.GetVmaAllocator(), framesInFlight);
    }

    SceneDrawSystemsManager::~SceneDrawSystemsManager()
    {
        auto& vulkanResource = _contextResources.GetVulkanResource();

        for (auto& sem : _updateSemaphores) {
            vkDestroySemaphore(vulkanResource.GetDevice(), sem, nullptr);
        }
        for (auto& sem : _renderSemaphores) {
            vkDestroySemaphore(vulkanResource.GetDevice(), sem, nullptr);
        }
    }

    void SceneDrawSystemsManager::Update(float deltaTime, Resources::Scene& scene)
    {
        auto& vulkanResource                = _contextResources.GetVulkanResource();
        auto& swapchainBoundResourceStorage = vulkanResource.GetSwapchainBoundStorage();

        auto& currentUpdateSemaphore = _updateSemaphores.at(vulkanResource.GetCurrentFrameOffset());
        auto& currentRenderSemaphore = _renderSemaphores.at(vulkanResource.GetCurrentFrameOffset());

        auto imageAcquiredSemaphore = vulkanResource.GetCurrentImageAcquiredSemaphore();

        auto currentFence = vulkanResource.GetCurrentFence();

        auto& currentCommandPoolResource = _commandPools.at(vulkanResource.GetCurrentFrameOffset());
        auto& currentStagingBuffer       = _stagingBuffers.at(vulkanResource.GetCurrentFrameOffset());

        currentCommandPoolResource.Reset();

        auto renderTargetOpt =
            swapchainBoundResourceStorage.Get<Resources::RenderTargetResource>(RENDER_TARGET_RESOURCE_ID, vulkanResource.GetCurrentFrameOffset());
        if (!renderTargetOpt) {
            uint32_t flags = 0;
            flags |= Resources::RenderTargetResource::RenderTargetCreationFlags::COLOR_ATTACHMENT;
            flags |= Resources::RenderTargetResource::RenderTargetCreationFlags::DEPTH_STENCIL_ATTACHMENT;

            auto renderTarget = std::make_unique<Resources::RenderTargetResource>(
                vulkanResource.GetDevice(), vulkanResource.GetVmaAllocator(), vulkanResource.GetSwapchainExtent(), flags);

            swapchainBoundResourceStorage.Insert<Resources::RenderTargetResource>(
                RENDER_TARGET_RESOURCE_ID, std::move(renderTarget), vulkanResource.GetCurrentFrameOffset());
            renderTargetOpt =
                swapchainBoundResourceStorage.Get<Resources::RenderTargetResource>(RENDER_TARGET_RESOURCE_ID, vulkanResource.GetCurrentFrameOffset());
        }
        auto& renderTarget = renderTargetOpt->get();

        { // Update
            auto commandBuffersScope = currentCommandPoolResource.BeginScope();

            _meshLoadingSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), currentStagingBuffer, scene);

            _screenClearingSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), currentStagingBuffer, scene);
            _meshDrawingSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), currentStagingBuffer, scene);
            _aabbDrawingSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), currentStagingBuffer, scene);
            _uiDrawingSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), currentStagingBuffer, scene);
            _gizmoDrawingSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), currentStagingBuffer, scene);
            _presentSystem.Update(deltaTime, commandBuffersScope.GetNextCommandBuffer(), currentStagingBuffer, scene);

            currentStagingBuffer.Commit(commandBuffersScope.GetNextCommandBuffer());

            VkSubmitInfo submitInfo{};
            submitInfo.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount     = 0;
            submitInfo.pWaitSemaphores        = nullptr;
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
            submitInfo.pWaitDstStageMask      = waitStages;
            submitInfo.commandBufferCount     = static_cast<uint32_t>(commandBuffersScope.size());
            submitInfo.pCommandBuffers        = commandBuffersScope.data();
            submitInfo.signalSemaphoreCount   = 1;
            submitInfo.pSignalSemaphores      = &currentUpdateSemaphore;

            vkQueueSubmit(vulkanResource.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        }

        { // Render
            auto commandBuffersScope = currentCommandPoolResource.BeginScope();

            _screenClearingSystem.Render(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene, renderTarget);
            _meshDrawingSystem.Render(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene, renderTarget);
            _aabbDrawingSystem.Render(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene, renderTarget);
            _uiDrawingSystem.Render(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene, renderTarget);
            _gizmoDrawingSystem.Render(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene, renderTarget);
            _presentSystem.Render(deltaTime, commandBuffersScope.GetNextCommandBuffer(), scene, renderTarget);

            VkSubmitInfo submitInfo{};
            submitInfo.sType                      = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkSemaphore          waitSemaphores[] = {currentUpdateSemaphore, imageAcquiredSemaphore};
            VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
            submitInfo.waitSemaphoreCount         = 2;
            submitInfo.pWaitSemaphores            = waitSemaphores;
            submitInfo.pWaitDstStageMask          = waitStages;
            submitInfo.commandBufferCount         = static_cast<uint32_t>(commandBuffersScope.size());
            submitInfo.pCommandBuffers            = commandBuffersScope.data();
            submitInfo.signalSemaphoreCount       = 1;
            VkSemaphore signalSemaphores[]        = {currentRenderSemaphore};
            submitInfo.pSignalSemaphores          = signalSemaphores;

            vkQueueSubmit(vulkanResource.GetGraphicsQueue(), 1, &submitInfo, currentFence);
        }

        { // Present
            auto currentImageIndex = vulkanResource.GetCurrentImageIndex();

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores    = &currentRenderSemaphore;
            VkSwapchainKHR swapchains[]    = {vulkanResource.GetSwapchain()};
            presentInfo.swapchainCount     = 1;
            presentInfo.pSwapchains        = swapchains;
            presentInfo.pImageIndices      = &currentImageIndex;

            vkQueuePresentKHR(vulkanResource.GetPresentationQueue(), &presentInfo);
        }
    }
} // namespace Prism::Managers