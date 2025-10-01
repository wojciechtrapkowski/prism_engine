#pragma once

#include "resources/context_resources.hpp"

#include "events/app_events.hpp"

#include "entt/entt.hpp"

namespace Prism::Systems {
    struct WindowResizeSystem {
      public:
        WindowResizeSystem(Resources::ContextResources &contextResources);
        ~WindowResizeSystem() = default;

        WindowResizeSystem(WindowResizeSystem &other) = delete;
        WindowResizeSystem &operator=(WindowResizeSystem &other) = delete;

        WindowResizeSystem(WindowResizeSystem &&other) = delete;
        WindowResizeSystem &operator=(WindowResizeSystem &&other) = delete;

        void Initialize();

        void Update(float deltaTime);

      private:
        Resources::ContextResources &m_contextResources;

        entt::scoped_connection m_onWindowResizeEventScopedConnection;
        std::optional<std::pair<int, int>> newWindowExtentOpt;

        void onWindowResizeEvent(Events::WindowResizeEvent event);
    };
}; // namespace Prism::Systems