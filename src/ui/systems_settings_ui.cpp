#include "ui/systems_settings_ui.hpp"

#include <numeric>
#include <string>
#include <format>

#include <imgui.h>
#include <imgui_internal.h>

#include "components/mesh.hpp"
#include "components/tags.hpp"
#include "components/transform.hpp"
#include "components/systems_settings.hpp"

namespace Prism::UI
{

    SystemsSettingsUI::SystemsSettingsUI(Resources::ContextResources& contextResources) : _contextResources(contextResources)
    {
        _onCpuRaytraceFinishedScopedConnection =
            _contextResources.GetDispatcher().sink<Events::CPURaytraceFinishedEvent>().connect<&SystemsSettingsUI::onCPURaytraceFinishEvent>(this);
    };

    void SystemsSettingsUI::Update(float deltaTime, Resources::Scene& scene)
    {
        if (_cpuRaytraceFinishedOpt) {
            _cpuRaytraceResults.push_back(*_cpuRaytraceFinishedOpt);
        }
        _cpuRaytraceFinishedOpt = std::nullopt;

        ImGui::Begin("Systems Settings");

        auto& vulkan = _contextResources.GetVulkanResource();

        auto& registry = scene.GetRegistry();

        auto meshDrawingSystemSettingsView = registry.view<Components::MeshDrawingSystemSettings>();
        if (!meshDrawingSystemSettingsView.empty()) {
            ImGui::BeginGroup();
            ImGui::Text("Mesh Drawing System Settings");

            auto& meshDrawingSystemSettings = meshDrawingSystemSettingsView.get<Components::MeshDrawingSystemSettings>(meshDrawingSystemSettingsView.front());

            std::vector<const char*> drawingModeItems = {"Rasterization"};

            drawingModeItems.push_back("Custom Raytracing");

            if (vulkan.GetAdditionalExtensions() & Resources::VulkanDeviceAdditionalExtensions::RAYTRACING_AVAILABLE) {
                drawingModeItems.push_back("API Raytracing");
            }

            int currentItem = static_cast<int>(meshDrawingSystemSettings.drawingMode);
            if (ImGui::Combo("Mesh Drawing Mode", &currentItem, drawingModeItems.data(), static_cast<int>(drawingModeItems.size()))) {
                meshDrawingSystemSettings.drawingMode = static_cast<Components::MeshDrawingSystemSettings::MeshDrawingMode>(currentItem);
            }

            if (meshDrawingSystemSettings.drawingMode == Components::MeshDrawingSystemSettings::MeshDrawingMode::CUSTOM_RAYTRACING) {
                // Button to trace
                if (ImGui::Button("Run ray trace")) {
                    _contextResources.GetDispatcher().enqueue<Events::CPURaytraceStartEvent>();
                }

                ImGui::SliderInt("Raytracing Width", &meshDrawingSystemSettings.customRaytracedGeometryDrawingSystemSettings.raytracingWidth, 1, 3840);
                ImGui::SliderInt("Raytracing Height", &meshDrawingSystemSettings.customRaytracedGeometryDrawingSystemSettings.raytracingHeight, 1, 2160);
            }

            // Always visible, so we can compare.
            if (!_cpuRaytraceResults.empty()) {
                std::vector<std::string> cpuRaytraceResultsLabelsStr(_cpuRaytraceResults.size());
                for (int i = 0; i < cpuRaytraceResultsLabelsStr.size(); i++) {
                    cpuRaytraceResultsLabelsStr[i] = std::format("Trace {}", i).c_str();
                }

                std::vector<const char*> cpuRaytraceResultsLabels(cpuRaytraceResultsLabelsStr.size());
                for (int i = 0; i < cpuRaytraceResultsLabels.size(); i++) {
                    cpuRaytraceResultsLabels[i] = cpuRaytraceResultsLabelsStr[i].c_str();
                }

                int& currentTrace = meshDrawingSystemSettings.customRaytracedGeometryDrawingSystemSettings.selectedCPURaytraceResult;

                if (ImGui::Combo("Traces", &currentTrace, cpuRaytraceResultsLabels.data(), static_cast<int>(cpuRaytraceResultsLabels.size()))) {
                }

                if (ImGui::Button("Open")) {
                    _isTraceOpen = true;
                }

                if (_isTraceOpen) {
                    if (ImGui::Begin("Raytrace Result", &_isTraceOpen)) {
                        auto&  traceResult = _cpuRaytraceResults[currentTrace];
                        ImVec2 imageSize(traceResult.width, traceResult.height); 

                        ImGui::Image((ImTextureID)traceResult.textureId, imageSize);
                    }
                    ImGui::End();
                }
            }

            ImGui::EndGroup();
        }
        
        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();

        auto aabbDrawingSystemSettingsView = registry.view<Components::AABBDrawingSystemSettings>();
        if (!aabbDrawingSystemSettingsView.empty()) {
            ImGui::BeginGroup();

            ImGui::Text("AABB Drawing System Settings");

            auto& aabbDrawingSystemSettings = registry.get<Components::AABBDrawingSystemSettings>(aabbDrawingSystemSettingsView.front());

            ImGui::Checkbox("Disable system", &aabbDrawingSystemSettings.disable);
            

            ImGui::EndGroup();
        }

        ImGui::End();
    }

    void SystemsSettingsUI::onCPURaytraceFinishEvent(Events::CPURaytraceFinishedEvent& event)
    {
        _cpuRaytraceFinishedOpt = event;
    }
} // namespace Prism::UI