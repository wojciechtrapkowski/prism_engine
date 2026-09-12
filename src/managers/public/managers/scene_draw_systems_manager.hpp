#pragma once

#include "systems/mesh_loading_system.hpp"

#include "systems/gizmo_drawing_system.hpp"
#include "systems/mesh_drawing_system.hpp"
#include "systems/aabb_drawing_system.hpp"
#include "systems/present_system.hpp"
#include "systems/screen_clearing_system.hpp"
#include "systems/ui_drawing_system.hpp"

#include "resources/context_resources.hpp"
#include "resources/render_target_resource.hpp"
#include "resources/resource.hpp"
#include "resources/scene.hpp"
#include "resources/vulkan/vk_command_pool_resource.hpp"
#include "resources/vulkan/vk_staging_buffer_resource.hpp"

namespace Prism::Managers
{
    class SceneDrawSystemsManager
    {
    public:
        SceneDrawSystemsManager(Resources::ContextResources& contextResources);

        ~SceneDrawSystemsManager();

        SceneDrawSystemsManager(const SceneDrawSystemsManager&)            = delete;
        SceneDrawSystemsManager& operator=(const SceneDrawSystemsManager&) = delete;

        SceneDrawSystemsManager(SceneDrawSystemsManager&&)            = delete;
        SceneDrawSystemsManager& operator=(SceneDrawSystemsManager&&) = delete;

        void Update(float deltaTime, Resources::Scene& scene);

    private:
        inline static const size_t   RENDER_TARGET_RESOURCE_ID = std::hash<std::string_view>{}("SceneDrawSystemsManager/RenderTargetResource");
        Resources::ContextResources& _contextResources;

        Systems::MeshLoadingSystem    _meshLoadingSystem;
        Systems::AABBDrawingSystem    _aabbDrawingSystem;
        Systems::ScreenClearingSystem _screenClearingSystem;
        Systems::MeshDrawingSystem    _meshDrawingSystem;
        Systems::GizmoDrawingSystem   _gizmoDrawingSystem;
        Systems::UIDrawingSystem      _uiDrawingSystem;
        Systems::PresentSystem        _presentSystem;

        // That is temporary, need a place for that. This is per frame in flight.
        std::vector<Resources::VkCommandPoolResource> _commandPools     = {};
        std::vector<VkSemaphore>                      _updateSemaphores = {};
        std::vector<VkSemaphore>                      _renderSemaphores = {};

        std::vector<Resources::VkStagingBufferResource> _stagingBuffers = {};
    };

} // namespace Prism::Managers