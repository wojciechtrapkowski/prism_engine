#pragma once

#include "systems/gizmo_drawing_system.hpp"
#include "systems/mesh_drawing_system.hpp"
#include "systems/present_system.hpp"
#include "systems/screen_clearing_system.hpp"
#include "systems/ui_drawing_system.hpp"

#include "resources/context_resources.hpp"
#include "resources/render_target_resource.hpp"
#include "resources/resource.hpp"
#include "resources/scene.hpp"
#include "resources/vulkan/vk_command_pool_resource.hpp"
#include "resources/vulkan/vk_staging_buffer_resource.hpp"

namespace Prism::Managers {
    class SceneDrawSystemsManager {
      public:
        SceneDrawSystemsManager(Resources::ContextResources &contextResources);

        ~SceneDrawSystemsManager();

        SceneDrawSystemsManager(const SceneDrawSystemsManager &) = delete;
        SceneDrawSystemsManager &operator=(const SceneDrawSystemsManager &) = delete;

        SceneDrawSystemsManager(SceneDrawSystemsManager &&) = delete;
        SceneDrawSystemsManager &operator=(SceneDrawSystemsManager &&) = delete;

        void Initialize();

        void Update(float deltaTime, Resources::Scene &scene, Resources::VkStagingBufferResource &stagingBuffer);

      private:
        inline static const size_t RENDER_TARGET_RESOURCE_ID = std::hash<std::string_view>{}("SceneDrawSystemsManager/RenderTargetResource");
        Resources::ContextResources &m_contextResources;

        Systems::ScreenClearingSystem screenClearingSystem;
        Systems::MeshDrawingSystem meshDrawingSystem;
        Systems::GizmoDrawingSystem gizmoDrawingSystem;
        Systems::UIDrawingSystem uiDrawingSystem;
        Systems::PresentSystem presentSystem;

        // That is temporary, need a place for that. This is per frame in flight.
        std::vector<Resources::VkCommandPoolResource> m_commandPools = {};
        std::vector<VkSemaphore> m_updateSemaphores = {};
        std::vector<VkSemaphore> m_renderSemaphores = {};
    };

} // namespace Prism::Managers