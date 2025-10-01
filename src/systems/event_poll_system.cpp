#include "systems/event_poll_system.hpp"

#include "imgui.h"

#include "events/move_events.hpp"

namespace Prism::Systems {
    namespace {}; // namespace

    EventPollSystem::EventPollSystem(Resources::ContextResources &contextResources) : m_contextResources(contextResources) {};

    void EventPollSystem::Initialize() {

    };

    void EventPollSystem::Update(float deltaTime) {
        glfwPollEvents();

        auto &dispatcher = m_contextResources.GetDispatcher();

        ImGuiIO &io = ImGui::GetIO();

        if (io.WantCaptureMouse) {
            dispatcher.clear<Events::MouseMoveEvent>();
            dispatcher.clear<Events::MouseButtonPressEvent>();
        }

        if (io.WantCaptureKeyboard) {
            dispatcher.clear<Events::KeyPressEvent>();
        }

        dispatcher.update();
    };
} // namespace Prism::Systems