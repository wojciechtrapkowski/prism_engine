#pragma once

#include "resources/context_resources.hpp"

#include "resources/context_resources.hpp"
#include "resources/render_target_resource.hpp"
#include "resources/scene.hpp"
#include "resources/vulkan/vk_staging_buffer_resource.hpp"

#include "volk/volk.h"

#include <vector>

namespace Prism::Systems
{
    class LightDrawingSystem
    {
    public:
        LightDrawingSystem(Resources::ContextResources& contextResources);
        ~LightDrawingSystem();

        LightDrawingSystem(LightDrawingSystem&& other)           = delete;
        LightDrawingSystem& operator=(LightDrawingSystem& other) = delete;

        LightDrawingSystem(LightDrawingSystem& other)             = delete;
        LightDrawingSystem& operator=(LightDrawingSystem&& other) = delete;

        void Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::VkStagingBufferResource& stagingBuffer, Resources::Scene& scene);

        void Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget);

        struct FragmentShaderPushConstants
        {
            glm::vec3 position;
            float     strength;
        };

    private:
        Resources::ContextResources& _contextResources;

        VkDescriptorPool             _descriptorPool      = VK_NULL_HANDLE;
        VkDescriptorSetLayout        _descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> _descriptorSets      = {};
        VkPipelineLayout             _pipelineLayout      = VK_NULL_HANDLE;
        VkPipeline                   _pipeline            = VK_NULL_HANDLE;
    };
} // namespace Prism::Systems
