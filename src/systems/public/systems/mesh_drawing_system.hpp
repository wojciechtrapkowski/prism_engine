#pragma once

#include "resources/context_resources.hpp"
#include "resources/render_target_resource.hpp"
#include "resources/scene.hpp"

namespace Prism::Systems {
    class MeshDrawingSystem {
      public:
        MeshDrawingSystem(Resources::ContextResources &contextResources);
        ~MeshDrawingSystem();

        MeshDrawingSystem(MeshDrawingSystem &other) = delete;
        MeshDrawingSystem &operator=(MeshDrawingSystem &other) = delete;

        MeshDrawingSystem(MeshDrawingSystem &&other) = delete;
        MeshDrawingSystem &operator=(MeshDrawingSystem &&other) = delete;

        void Initialize();

        void Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene &scene);

        void Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene &scene, Resources::RenderTargetResource &renderTarget);

      private:
        Resources::ContextResources &m_contextResources;

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets = {};
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
    };
}; // namespace Prism::Systems