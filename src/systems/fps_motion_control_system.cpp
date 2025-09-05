#include "systems/fps_motion_control_system.hpp"

#include "components/camera.hpp"
#include "components/fps_camera_control.hpp"
#include "components/tags.hpp"
#include "components/transform.hpp"

#include "events/move_events.hpp"

#include "utils/opengl_debug.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <iostream>

namespace Prism::Systems {
    namespace {
        constexpr float PITCH_LIMIT = 89.0f;
        constexpr glm::vec3 WORLD_FORWARD_VECTOR = {0.0f, 0.0f, -1.0f};
        constexpr glm::vec3 WORLD_RIGHT_VECTOR = {1.0f, 0.0f, 0.0f};
        constexpr glm::vec3 WORLD_UP_VECTOR = {0.0f, 1.0f, 0.0f};
    } // namespace

    FpsMotionControlSystem::FpsMotionControlSystem(
        Resources::ContextResources &contextResources)
        : m_contextResources(contextResources) {
        auto &dispatcher = m_contextResources.dispatcher;

        m_onKeyPressedConnection =
            dispatcher.sink<Events::KeyPressEvent>()
                .connect<&FpsMotionControlSystem::onKeyPressed>(this);

        m_onMousePressedConnection =
            dispatcher.sink<Events::MouseButtonPressEvent>()
                .connect<&FpsMotionControlSystem::onMousePressed>(this);

        m_onMouseMovementConnection =
            dispatcher.sink<Events::MouseMoveEvent>()
                .connect<&FpsMotionControlSystem::onMouseMoved>(this);
    };

    void FpsMotionControlSystem::Initialize() {

    };

    void FpsMotionControlSystem::Update(float deltaTime,
                                        Resources::Scene &scene) {
        auto &registry = scene.GetRegistry();

        auto activeCameraView = registry.view<Components::Tags::ActiveCamera>();
        if (activeCameraView.empty()) {
            return;
        }

        if (!registry.all_of<Components::Camera, Components::FpsCameraControl,
                             Components::Transform>(activeCameraView.front())) {
            return;
        }

        auto fpsCamera = activeCameraView.front();

        auto &camera = registry.get<Components::Camera>(fpsCamera);
        auto &cameraControl =
            registry.get<Components::FpsCameraControl>(fpsCamera);
        auto &transform = registry.get<Components::Transform>(fpsCamera);

        auto cameraPosition = glm::vec3(transform.transform[3]);
        auto cameraRotation = glm::mat4(glm::mat3(transform.transform));

        float pitch, yaw, roll;

        glm::extractEulerAngleYXZ(cameraRotation, yaw, pitch, roll);

        pitch = glm::degrees(pitch);
        yaw = glm::degrees(yaw);
        roll = glm::degrees(roll);

        if (m_mouseButtonToStateMap[Events::MoveEvents::MouseButton::Left] ==
            Events::MoveEvents::InputAction::Pressed) {
            float deltaX =
                m_mousePositionDelta.first * cameraControl.mouseSensitivity;
            float deltaY =
                m_mousePositionDelta.second * cameraControl.mouseSensitivity;

            yaw -= deltaX;
            pitch += deltaY;

            pitch = glm::clamp(pitch, -PITCH_LIMIT, PITCH_LIMIT);
        }

        yaw = glm::radians(yaw);
        pitch = glm::radians(pitch);

        cameraRotation = glm::eulerAngleYXZ(yaw, pitch, 0.0f);

        auto cameraRight = glm::vec3(cameraRotation[0]);
        auto cameraUp = glm::vec3(cameraRotation[1]);
        auto cameraForward = -glm::vec3(cameraRotation[2]);

        if (m_keyToStateMap[Events::MoveEvents::Keys::W] ==
            Events::MoveEvents::InputAction::Pressed) {
            cameraPosition +=
                cameraForward * cameraControl.moveSpeed * deltaTime;
        }
        if (m_keyToStateMap[Events::MoveEvents::Keys::S] ==
            Events::MoveEvents::InputAction::Pressed) {
            cameraPosition -=
                cameraForward * cameraControl.moveSpeed * deltaTime;
        }
        if (m_keyToStateMap[Events::MoveEvents::Keys::A] ==
            Events::MoveEvents::InputAction::Pressed) {
            cameraPosition -= cameraRight * cameraControl.moveSpeed * deltaTime;
        }
        if (m_keyToStateMap[Events::MoveEvents::Keys::D] ==
            Events::MoveEvents::InputAction::Pressed) {
            cameraPosition += cameraRight * cameraControl.moveSpeed * deltaTime;
        }
        if (m_keyToStateMap[Events::MoveEvents::Keys::SPACE] ==
            Events::MoveEvents::InputAction::Pressed) {
            cameraPosition +=
                WORLD_UP_VECTOR * cameraControl.moveSpeed * deltaTime;
        }
        if (m_keyToStateMap[Events::MoveEvents::Keys::SHIFT] ==
            Events::MoveEvents::InputAction::Pressed) {
            cameraPosition -=
                WORLD_UP_VECTOR * cameraControl.moveSpeed * deltaTime;
        }

        glm::mat4 rotationMat = cameraRotation;
        glm::mat4 translationMat =
            glm::translate(glm::mat4(1.0f), cameraPosition);
        transform.transform = translationMat * rotationMat;

        glm::mat4 view = glm::lookAt(cameraPosition,
                                     cameraPosition + cameraForward, cameraUp);

        int width, height;
        GLCheck(glfwGetFramebufferSize(m_contextResources.window.get(), &width,
                                       &height));
        glViewport(0, 0, width, height);

        float aspectRatio =
            static_cast<float>(width) / static_cast<float>(height);

        glm::mat4 projection =
            glm::perspective(glm::radians(cameraControl.fov), aspectRatio,
                             cameraControl.nearPlane, cameraControl.farPlane);

        camera.view = std::move(view);
        camera.projection = std::move(projection);

        m_mousePositionDelta = {0.f, 0.f};
        m_keyToStateMap.clear();
        m_mouseButtonToStateMap.clear();
    }

    void
    FpsMotionControlSystem::onKeyPressed(const Events::KeyPressEvent &event) {
        m_keyToStateMap[event.key] = event.action;
    }

    void FpsMotionControlSystem::onMousePressed(
        const Events::MouseButtonPressEvent &event) {
        m_mouseButtonToStateMap[event.button] = event.action;
    }

    void
    FpsMotionControlSystem::onMouseMoved(const Events::MouseMoveEvent &event) {

        m_mousePositionDelta = {event.position.first - m_mousePosition.first,
                                m_mousePosition.second - event.position.second};

        // Skip first update.
        if (m_mousePosition == std::pair<double, double>{0, 0}) {
            m_mousePositionDelta = {0, 0};
        }
        m_mousePosition = {event.position.first, event.position.second};
    }

} // namespace Prism::Systems
