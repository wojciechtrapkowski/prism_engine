#pragma once

#include "resources/context_resources.hpp"
#include "resources/render_target_resource.hpp"
#include "resources/scene.hpp"

#include "events/move_events.hpp"

namespace Prism::Systems {
    class GizmoDrawingSystem {
      public:
        GizmoDrawingSystem(Resources::ContextResources &contextResources);
        ~GizmoDrawingSystem() = default;

        GizmoDrawingSystem(GizmoDrawingSystem &other) = delete;
        GizmoDrawingSystem &operator=(GizmoDrawingSystem &other) = delete;

        GizmoDrawingSystem(GizmoDrawingSystem &&other) = delete;
        GizmoDrawingSystem &operator=(GizmoDrawingSystem &&other) = delete;

        void Initialize();

        void Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene &scene);

        void Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene &scene, Resources::RenderTargetResource &renderTarget);

      private:
        void onKeyPressed(const Events::KeyPressEvent &event);

        Resources::ContextResources &m_contextResources;

        entt::scoped_connection m_onKeyPressedConnection;

        std::unordered_map<Events::Keys, Events::InputAction> m_keyToStateMap;
    };
}; // namespace Prism::Systems