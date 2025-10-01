#include "systems/input_control_system.hpp"

#include "events/app_events.hpp"
#include "events/move_events.hpp"

namespace Prism::Systems {
    namespace {
        void handleWindowClose(entt::dispatcher &dispatcher, Resources::WindowResource &windowResource) {
            auto window = windowResource.GetWindow();
            if (glfwWindowShouldClose(window)) {
                dispatcher.enqueue<Events::WindowCloseEvent>(Events::WindowCloseEvent{});
            }
        }

        void handleWindowResize(entt::dispatcher &dispatcher, Resources::WindowResource &windowResource) {
            auto [currentWidth, currentHeight] = windowResource.GetWindowExtent();

            int width, height;
            glfwGetFramebufferSize(windowResource.GetWindow(), &width, &height);
            if (width == 0 || height == 0) {
                return;
            }

            if (width != currentWidth || height != currentHeight) {
                dispatcher.enqueue<Events::WindowResizeEvent>(Events::WindowResizeEvent{.newWidth = width, .newHeight = height});
                windowResource.UpdateWindowExtent(width, height);
            }
        }

        void handleKeyPress(entt::dispatcher &dispatcher, GLFWwindow *window) {
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                dispatcher.enqueue<Events::KeyPressEvent>(
                    Events::KeyPressEvent{.action = Events::MoveEvents::InputAction::Pressed, .key = Events::MoveEvents::Keys::SPACE});
            }

            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                dispatcher.enqueue<Events::KeyPressEvent>(
                    Events::KeyPressEvent{.action = Events::MoveEvents::InputAction::Pressed, .key = Events::MoveEvents::Keys::SHIFT});
            }

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                dispatcher.enqueue<Events::KeyPressEvent>(
                    Events::KeyPressEvent{.action = Events::MoveEvents::InputAction::Pressed, .key = Events::MoveEvents::Keys::W});
            }

            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                dispatcher.enqueue<Events::KeyPressEvent>(
                    Events::KeyPressEvent{.action = Events::MoveEvents::InputAction::Pressed, .key = Events::MoveEvents::Keys::S});
            }

            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                dispatcher.enqueue<Events::KeyPressEvent>(
                    Events::KeyPressEvent{.action = Events::MoveEvents::InputAction::Pressed, .key = Events::MoveEvents::Keys::A});
            }

            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                dispatcher.enqueue<Events::KeyPressEvent>(
                    Events::KeyPressEvent{.action = Events::MoveEvents::InputAction::Pressed, .key = Events::MoveEvents::Keys::D});
            }

            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                dispatcher.enqueue<Events::KeyPressEvent>(
                    Events::KeyPressEvent{.action = Events::MoveEvents::InputAction::Pressed, .key = Events::MoveEvents::Keys::R});
            }

            if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
                dispatcher.enqueue<Events::KeyPressEvent>(
                    Events::KeyPressEvent{.action = Events::MoveEvents::InputAction::Pressed, .key = Events::MoveEvents::Keys::T});
            }
        }

        void handleMouseMovement(entt::dispatcher &dispatcher, GLFWwindow *window) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            dispatcher.enqueue<Events::MouseMoveEvent>(Events::MouseMoveEvent{.position = {xpos, ypos}});
        }

        void handleMouseButtonPress(entt::dispatcher &dispatcher, GLFWwindow *window) {
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                dispatcher.enqueue<Events::MouseButtonPressEvent>(
                    Events::MouseButtonPressEvent{.action = Events::MoveEvents::InputAction::Pressed, .button = Events::MoveEvents::MouseButton::Right});
            }
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                dispatcher.enqueue<Events::MouseButtonPressEvent>(
                    Events::MouseButtonPressEvent{.action = Events::MoveEvents::InputAction::Pressed, .button = Events::MoveEvents::MouseButton::Left});
            }
        }
    } // namespace

    InputControlSystem::InputControlSystem(Resources::ContextResources &contextResources) : m_contextResources(contextResources) {};

    void InputControlSystem::Initialize() {};

    void InputControlSystem::Update(float deltaTime) {
        auto &dispatcher = m_contextResources.GetDispatcher();
        auto &window = m_contextResources.GetWindowResource();
        auto windowPtr = window.GetWindow();

        handleWindowClose(dispatcher, window);
        handleWindowResize(dispatcher, window);

        handleKeyPress(dispatcher, windowPtr);
        handleMouseMovement(dispatcher, windowPtr);
        handleMouseButtonPress(dispatcher, windowPtr);

        if (glfwGetKey(windowPtr, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(windowPtr, true);
        }
    };
} // namespace Prism::Systems