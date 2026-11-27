#include "ui/systems_settings_ui.hpp"

#include <numeric>
#include <string>
#include <format>

#include <imgui.h>
#include <imgui_internal.h>

#include "resources/cpu_raytrace_result_resource.hpp"

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

                std::vector<const char*> builderItems = {"Morton", "SAH"};
                int                      currentItem  = static_cast<int>(meshDrawingSystemSettings.customRaytracedGeometryDrawingSystemSettings.builder);
                if (ImGui::Combo("Builder", &currentItem, builderItems.data(), static_cast<int>(builderItems.size()))) {
                    meshDrawingSystemSettings.customRaytracedGeometryDrawingSystemSettings.builder =
                        static_cast<Components::MeshDrawingSystemSettings::CustomRaytracedGeometryDrawingSystemSettings::Builder>(currentItem);
                }
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

                if (ImGui::Combo("Traces", &_selectedCPURaytraceResult, cpuRaytraceResultsLabels.data(), static_cast<int>(cpuRaytraceResultsLabels.size()))) {
                }

                if (ImGui::Button("Open")) {
                    _isTraceOpen = true;
                }

                if (_isTraceOpen) {
                    auto& traceResult     = _cpuRaytraceResults[_selectedCPURaytraceResult];
                    auto  outputId        = traceResult.outputId;
                    auto& systemsStorage  = scene.GetSystemsStorage();
                    auto  outputResultOpt = systemsStorage.Get<Resources::CPURaytraceResultResource>(outputId);

                    if (!outputResultOpt) {
                        throw std::runtime_error("Couldn't get CPU raytrace result!");
                    }
                    auto& outputResult = outputResultOpt->get();

                    if (ImGui::Begin("Raytrace Result", &_isTraceOpen)) {
                        constexpr auto SCALE = 3;

                        ImDrawList* drawList = ImGui::GetWindowDrawList();

                        // Scale the image, so we can show per pixel statistics.
                        ImVec2 imageSize(traceResult.width * SCALE, traceResult.height * SCALE);

                        ImVec2 imagePosition = ImGui::GetCursorScreenPos();
                        ImGui::Image((ImTextureID)traceResult.textureId, imageSize);

                        if (ImGui::IsItemHovered()) {
                            auto mousePosX = ImGui::GetMousePos().x;
                            auto mousePosY = ImGui::GetMousePos().y;

                            // 0 0 is in the top left corner
                            // width, height is in the bottom right corner

                            auto selectedPixelX = std::abs(imagePosition.x - mousePosX);
                            auto selectedPixelY = std::abs(imagePosition.y - mousePosY);

                            ImVec2 startPosition = {imagePosition.x + selectedPixelX, imagePosition.y + selectedPixelY};
                            ImVec2 endPosition   = {startPosition.x + SCALE, startPosition.y + SCALE};

                            // Draw outline.
                            drawList->AddRect(startPosition, endPosition, IM_COL32(255, 255, 0, 255), 0.0f, 0, 1.0f);

                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                _traceSelectedPixelX = selectedPixelX / SCALE;
                                _traceSelectedPixelY = selectedPixelY / SCALE;
                            }

                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                                _traceSelectedPixelX = -1;
                                _traceSelectedPixelY = -1;
                            }

                            // std::cout << std::abs(imagePosition.x - mousePosX) << " " << std::abs(imagePosition.y - mousePosY) << std::endl;
                        }

                        bool pixelStatisticsOpen = true;
                        ImGui::Begin("Pixel statistics", &pixelStatisticsOpen);

                        if (_traceSelectedPixelX != -1 && _traceSelectedPixelY != -1) {
                            ImVec2 startPosition = {imagePosition.x + _traceSelectedPixelX * SCALE, imagePosition.y + _traceSelectedPixelY * SCALE};
                            ImVec2 endPosition   = {startPosition.x + SCALE, startPosition.y + SCALE};

                            drawList->AddRect(startPosition, endPosition, IM_COL32(255, 0, 0, 255));

                            auto& pixelData = outputResult.Data()[_traceSelectedPixelY * traceResult.width + _traceSelectedPixelX];

                            auto label = std::format("AABB intersections: {}\nTriangle hits count: {}\n", pixelData.aabbIntersections, pixelData.triangleHits);
                            ImGui::TextUnformatted(label.c_str());
                        }

                        ImGui::End(); // Pixel statistics

                        bool compareOpen = true;
                        ImGui::Begin("Compare", &compareOpen);

                        if (ImGui::Combo(
                                "Compare to", &_compareCPURaytraceResult, cpuRaytraceResultsLabels.data(), static_cast<int>(cpuRaytraceResultsLabels.size())))
                        {}

                        if (_compareCPURaytraceResult != -1) {
                            auto baseResultStatistics = outputResult.GetStatistics();

                            auto compareResultId  = _cpuRaytraceResults[_compareCPURaytraceResult];
                            auto compareResultOpt = systemsStorage.Get<Resources::CPURaytraceResultResource>(compareResultId.outputId);
                            if (!compareResultOpt) {
                                throw std::runtime_error("Couldn't get CPU compare raytrace reuslt!");
                            }
                            auto& compareResult = compareResultOpt->get();

                            auto compareResultStatistics = compareResult.GetStatistics();

                            if (ImGui::BeginTable("Comparision results", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                                auto baseTrace    = std::format("Trace {}", _selectedCPURaytraceResult);
                                auto compareTrace = std::format("Trace {}", _compareCPURaytraceResult);
;
                                ImGui::TableSetupColumn(""); 
                                ImGui::TableSetupColumn(baseTrace.c_str());
                                ImGui::TableSetupColumn(compareTrace.c_str());

                                ImGui::TableHeadersRow();

                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImGui::TableHeader("AABB intersections");

                                ImGui::TableNextColumn();
                                ImGui::Text("%d", baseResultStatistics.averageAABBIntersections);

                                ImGui::TableNextColumn();
                                ImGui::Text("%d", compareResultStatistics.averageAABBIntersections);

                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImGui::TableHeader("Triangle hits");

                                ImGui::TableNextColumn();
                                ImGui::Text("%d", baseResultStatistics.averageTriangleHits);

                                ImGui::TableNextColumn();
                                ImGui::Text("%d", compareResultStatistics.averageTriangleHits);

                                ImGui::EndTable();
                            }
                        }

                        ImGui::End(); // Compare open
                    }
                    ImGui::End();     // Raytrace Result - must be called even if Begin returned false
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

        ImGui::End(); // Systems Settings
    }

    void SystemsSettingsUI::onCPURaytraceFinishEvent(Events::CPURaytraceFinishedEvent& event)
    {
        _cpuRaytraceFinishedOpt = event;
    }
} // namespace Prism::UI