#include "systems/ui_drawing_system.hpp"

#include "events/move_events.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

namespace Prism::Systems {
    UIDrawingSystem::UIDrawingSystem(
        Resources::ContextResources &contextResources)
        : m_contextResources(contextResources), m_mainDockUI{contextResources},
          m_menuBarUI{contextResources}, m_sceneHierarchyUI{contextResources},
          m_cameraSettingsUI{contextResources} {};

    void UIDrawingSystem::Initialize() {}

    void UIDrawingSystem::Update(float deltaTime, Resources::Scene &scene) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::GetIO().WantCaptureMouse) {
            m_contextResources.dispatcher.clear<Events::MouseMoveEvent>();
        }

        m_mainDockUI.Update(deltaTime, scene);
        m_menuBarUI.Update(deltaTime, scene);
        m_sceneHierarchyUI.Update(deltaTime, scene);
        m_cameraSettingsUI.Update(deltaTime, scene);
    }

    void UIDrawingSystem::Render(float deltaTime, Resources::Scene &scene) {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        GLFWwindow *backupCurrentContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backupCurrentContext);
    }
} // namespace Prism::Systems