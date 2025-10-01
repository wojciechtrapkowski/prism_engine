#pragma once


#include <GLFW/glfw3.h>

#include "resources/context_resources.hpp"
#include "resources/scene.hpp"

#include "events/move_events.hpp"

namespace Prism::Systems {
    struct FpsMotionControlSystem {

        FpsMotionControlSystem(Resources::ContextResources &contextResources);
        ~FpsMotionControlSystem() = default;

        FpsMotionControlSystem(FpsMotionControlSystem &other) = delete;
        FpsMotionControlSystem &operator=(FpsMotionControlSystem &other) = delete;

        FpsMotionControlSystem(FpsMotionControlSystem &&other) = delete;
        FpsMotionControlSystem &operator=(FpsMotionControlSystem &&other) = delete;

        void Initialize();

        void Update(float deltaTime, Resources::Scene &scene);

      private:
        void onKeyPressed(const Events::KeyPressEvent &event);
        void onMousePressed(const Events::MouseButtonPressEvent &event);
        void onMouseMoved(const Events::MouseMoveEvent &event);

        Resources::ContextResources &m_contextResources;

        entt::scoped_connection m_onKeyPressedConnection;
        entt::scoped_connection m_onMousePressedConnection;
        entt::scoped_connection m_onMouseMovementConnection;

        std::unordered_map<Events::Keys, Events::InputAction> m_keyToStateMap;

        std::unordered_map<Events::MouseButton, Events::InputAction> m_mouseButtonToStateMap;


        std::pair<double, double> m_mousePosition = {};
        std::pair<double, double> m_mousePositionDelta = {};
    };
}; // namespace Prism::Systems