#pragma once

#include "resources/scene.hpp"

#include "resources/context_resources.hpp"

#include <entt/entt.hpp>

#include "events/app_events.hpp"

namespace Prism::Context {
    struct Context {
        Context();
        ~Context() = default;

        Context(Context &&other) = delete;
        Context &operator=(Context &&) = delete;

        Context(Context &other) = delete;
        Context &operator=(Context &) = delete;

        void RunEngine();

      private:
        void onWindowClose(Events::WindowCloseEvent &event);

        Resources::ContextResources m_contextResources;

        bool m_isRunning = true;
        entt::scoped_connection m_windowCloseEventConnection;

        entt::registry m_registry;
    };
}; // namespace Prism::Context