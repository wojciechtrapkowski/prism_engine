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
    class AABBDrawingSystem
    {
    public:
        AABBDrawingSystem(Resources::ContextResources& contextResources);
        ~AABBDrawingSystem();

        AABBDrawingSystem(AABBDrawingSystem&& other)           = delete;
        AABBDrawingSystem& operator=(AABBDrawingSystem& other) = delete;

        AABBDrawingSystem(AABBDrawingSystem& other)             = delete;
        AABBDrawingSystem& operator=(AABBDrawingSystem&& other) = delete;

        void Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::VkStagingBufferResource& stagingBuffer, Resources::Scene& scene);

        void Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget);

    private:
        struct BoxTransformEntry
        {
            glm::vec4 lower;
            glm::vec3 upper;
            uint32_t  entityId;
        };

        inline static const Resources::Resource::ID BOX_MESH_RESOURCE_ID     = std::hash<std::string_view>{}("AABBDrawingSystem/BoxMeshResourceId");
        inline static const Resources::Resource::ID BOX_TRANSFORMS_BUFFER_ID = std::hash<std::string_view>{}("AABBDrawingSystem/BoxTransformsBufferId");

        Resources::ContextResources& _contextResources;

        std::optional<Resources::VkBufferResource<BoxTransformEntry>> _transformsBufferToDelete = std::nullopt;

        VkDescriptorPool             _descriptorPool      = VK_NULL_HANDLE;
        VkDescriptorSetLayout        _descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> _descriptorSets      = {};
        VkPipelineLayout             _pipelineLayout      = VK_NULL_HANDLE;
        VkPipeline                   _pipeline            = VK_NULL_HANDLE;
    };
} // namespace Prism::Systems
