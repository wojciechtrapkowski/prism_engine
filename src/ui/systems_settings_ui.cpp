#include "ui/systems_settings_ui.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "components/mesh.hpp"
#include "components/tags.hpp"
#include "components/transform.hpp"
#include "components/systems_settings.hpp"

namespace Prism::UI
{

    SystemsSettingsUI::SystemsSettingsUI(Resources::ContextResources& contextResources) : _contextResources(contextResources) {};

    void SystemsSettingsUI::Update(float deltaTime, Resources::Scene& scene)
    {
        ImGui::Begin("Systems Settings");

        auto& vulkan = _contextResources.GetVulkanResource();

        auto& registry = scene.GetRegistry();

        auto meshDrawingSystemSettingsView = registry.view<Components::MeshDrawingSystemSettings>();
        if (!meshDrawingSystemSettingsView.empty()) {
            ImGui::BeginGroup();
            ImGui::Text("Mesh Drawing System Settings");

            auto& meshDrawingSystemSettings = meshDrawingSystemSettingsView.get<Components::MeshDrawingSystemSettings>(meshDrawingSystemSettingsView.front());

            std::vector<const char*> drawingModeItems = {"Rasterization"};

            if (vulkan.GetAdditionalExtensions() & Resources::VulkanDeviceAdditionalExtensions::RAYTRACING_AVAILABLE) {
                drawingModeItems.push_back("API Raytracing");
            }

            drawingModeItems.push_back("Custom Raytracing");

            int currentItem = static_cast<int>(meshDrawingSystemSettings.drawingMode);
            if (ImGui::Combo("Mesh Drawing Mode", &currentItem, drawingModeItems.data(), static_cast<int>(drawingModeItems.size()))) {
                meshDrawingSystemSettings.drawingMode = static_cast<Components::MeshDrawingSystemSettings::MeshDrawingMode>(currentItem);
            }
            ImGui::EndGroup();
        }

        ImGui::End();
    }
} // namespace Prism::UI