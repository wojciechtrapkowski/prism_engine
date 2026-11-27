#pragma once

#include "resources/context_resources.hpp"
#include "resources/render_target_resource.hpp"
#include "resources/scene.hpp"
#include "resources/vulkan/vk_staging_buffer_resource.hpp"

namespace Prism::Systems::Subsystems::MeshDrawingSystem
{
    class RasterizedGeometryDrawingSubsystem
    {
    public:
        RasterizedGeometryDrawingSubsystem(Resources::ContextResources& contextResources);
        ~RasterizedGeometryDrawingSubsystem();

        RasterizedGeometryDrawingSubsystem(RasterizedGeometryDrawingSubsystem& other)            = delete;
        RasterizedGeometryDrawingSubsystem& operator=(RasterizedGeometryDrawingSubsystem& other) = delete;

        RasterizedGeometryDrawingSubsystem(RasterizedGeometryDrawingSubsystem&& other)            = delete;
        RasterizedGeometryDrawingSubsystem& operator=(RasterizedGeometryDrawingSubsystem&& other) = delete;

        void Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::VkStagingBufferResource& stagingBuffer);

        void Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget);

        struct VertexShaderPushConstants
        {
            glm::mat4 model;
            int       textureIndex;
        };

        struct FragmentShaderPushConstants
        {
            int lightsCount;
        };

    private:
        struct LightEntry
        {
            glm::vec3 position;
            float     strength;
        };

        inline static const Resources::Resource::ID TEXTURES_SAMPLERS_BUFFER_ID =
            std::hash<std::string_view>{}("RasterizedGeometryDrawingSubsystem/TexturesSamplersBufferId");
        inline static const Resources::Resource::ID LIGHTS_BUFFER_ID = std::hash<std::string_view>{}("RasterizedGeometryDrawingSubsystem/LightsBufferId");

        Resources::ContextResources& _contextResources;

        VkDescriptorPool             descriptorPool      = VK_NULL_HANDLE;
        VkDescriptorSetLayout        descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets      = {};
        VkPipelineLayout             pipelineLayout      = VK_NULL_HANDLE;
        VkPipeline                   pipeline            = VK_NULL_HANDLE;

        std::optional<Resources::VkBufferResource<LightEntry>> _lightsBufferToDeleteOpt = std::nullopt;
    };
}; // namespace Prism::Systems::Subsystems::MeshDrawingSystem