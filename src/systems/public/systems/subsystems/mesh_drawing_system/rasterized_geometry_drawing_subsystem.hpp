#pragma once

#include "resources/context_resources.hpp"
#include "resources/render_target_resource.hpp"
#include "resources/scene.hpp"

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

        void Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene);

        void Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget);

        struct PushConstants
        {
            glm::mat4 model;
            int       textureIndex;
        };

    private:
        inline static const Resources::Resource::ID TEXTURES_SAMPLERS_BUFFER_ID =
            std::hash<std::string_view>{}("RasterizedGeometryDrawingSubsystem/TexturesSamplersBufferId");

        Resources::ContextResources& _contextResources;

        VkDescriptorPool             descriptorPool      = VK_NULL_HANDLE;
        VkDescriptorSetLayout        descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets      = {};
        VkPipelineLayout             pipelineLayout      = VK_NULL_HANDLE;
        VkPipeline                   pipeline            = VK_NULL_HANDLE;
    };
}; // namespace Prism::Systems::Subsystems::MeshDrawingSystem