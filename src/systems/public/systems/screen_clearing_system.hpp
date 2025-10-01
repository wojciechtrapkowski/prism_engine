#pragma once

#include "resources/context_resources.hpp"
#include "resources/render_target_resource.hpp"
#include "resources/scene.hpp"

namespace Prism::Systems {
    class ScreenClearingSystem {
      public:
        ScreenClearingSystem(Resources::ContextResources &contextResources);
        ~ScreenClearingSystem() = default;

        ScreenClearingSystem(ScreenClearingSystem &other) = delete;
        ScreenClearingSystem &operator=(ScreenClearingSystem &other) = delete;

        ScreenClearingSystem(ScreenClearingSystem &&other) = delete;
        ScreenClearingSystem &operator=(ScreenClearingSystem &&other) = delete;

        void Initialize();

        void Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene &scene);

        void Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene &scene, Resources::RenderTargetResource &renderTarget);

      private:
        Resources::ContextResources &m_contextResources;
    };
}; // namespace Prism::Systems