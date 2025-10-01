#include "systems/window_resize_system.hpp"

namespace Prism::Systems {
    namespace {} // namespace

    WindowResizeSystem::WindowResizeSystem(Resources::ContextResources &contextResources) : m_contextResources(contextResources) {
        auto &dispatcher = m_contextResources.GetDispatcher();
        m_onWindowResizeEventScopedConnection = dispatcher.sink<Events::WindowResizeEvent>().connect<&WindowResizeSystem::onWindowResizeEvent>(this);
    };

    void WindowResizeSystem::Initialize() {};

    void WindowResizeSystem::Update(float deltaTime) {
        if (newWindowExtentOpt != std::nullopt) {
            auto &vulkanResource = m_contextResources.GetVulkanResource();
            auto &newWindowExtent = *newWindowExtentOpt;

            auto [width, height] = newWindowExtent;
            newWindowExtentOpt = std::nullopt;

            if (width == 0 || height == 0) {
                return;
            }

            vulkanResource.RecreateSwapchain(width, height);
        }
    };

    void WindowResizeSystem::onWindowResizeEvent(Events::WindowResizeEvent event) { newWindowExtentOpt = {event.newWidth, event.newHeight}; }
} // namespace Prism::Systems