#include "ui/scene_hierarchy_ui.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "components/mesh.hpp"
#include "components/tags.hpp"
#include "components/transform.hpp"
#include "components/name.hpp"
#include "components/rigid_body.hpp"
#include "components/aabb.hpp"

#include <format>

namespace Prism::UI
{
    namespace
    {
        void renderTransformComponent(entt::registry& registry, entt::entity entity)
        {
            if (!registry.all_of<Components::Transform>(entity))
                return;

            auto isOpened = ImGui::TreeNode("Transform");

            ImGui::OpenPopupOnItemClick("##TransformContext", ImGuiPopupFlags_MouseButtonRight);
            if (ImGui::BeginPopup("##TransformContext")) {
                if (ImGui::MenuItem("Remove Transform")) {
                    registry.remove<Components::Transform>(entity);
                }
                ImGui::EndPopup();
            }

            if (isOpened) {
                // Because user could delete Transform component, we need to check if it still exists.
                auto transformPtr = registry.try_get<Components::Transform>(entity);
                if (!transformPtr) {
                    ImGui::TreePop();
                    return;
                }
                auto& transform = transformPtr->transform;

                if (ImGui::BeginTable("TransformMatrix", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("X");
                    ImGui::TableSetupColumn("Y");
                    ImGui::TableSetupColumn("Z");
                    ImGui::TableSetupColumn("W");
                    ImGui::TableHeadersRow();

                    for (int row = 0; row < 4; ++row) {
                        ImGui::TableNextRow();
                        for (int col = 0; col < 4; ++col) {
                            ImGui::TableSetColumnIndex(col);
                            std::string label = "##M_" + std::to_string(row) + "_" + std::to_string(col);
                            ImGui::InputFloat(label.c_str(), &transform[row][col], 0.1f, 1.0f, "%.3f");
                        }
                    }

                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }
        }

        void renderMeshComponent(
            const std::vector<std::reference_wrapper<Resources::MeshResource>>& availableMeshResources, entt::registry& registry, entt::entity entity)
        {
            std::vector<std::pair<Resources::MeshResource::ID, std::string>> availableMeshResourcesIdsAndNames;

            // One additional place for uninitialized field.
            availableMeshResourcesIdsAndNames.reserve(availableMeshResources.size() + 1);

            availableMeshResourcesIdsAndNames.push_back(std::make_pair(Resources::MeshResource::UNINITIALIZED_ID, "None"));

            for (const auto& availableMeshResourceRef : availableMeshResources) {
                auto& availableMeshResource     = availableMeshResourceRef.get();
                auto  availableMeshResourceId   = availableMeshResource.GetID();
                auto  availableMeshResourceName = std::format("Mesh name: {}", availableMeshResource.GetName());

                if (availableMeshResourceId == std::nullopt) {
                    continue;
                }

                availableMeshResourcesIdsAndNames.push_back(std::make_pair(availableMeshResourceId.value(), availableMeshResourceName));
            }

            if (!registry.all_of<Components::Mesh>(entity))
                return;

            auto isOpened = ImGui::TreeNode("Mesh");

            ImGui::OpenPopupOnItemClick("##MeshContext", ImGuiPopupFlags_MouseButtonRight);
            if (ImGui::BeginPopup("##MeshContext")) {
                if (ImGui::MenuItem("Remove Mesh")) {
                    registry.remove<Components::Mesh>(entity);
                }
                ImGui::EndPopup();
            }

            if (isOpened) {
                // Because user could delete Mesh component, we need to check if it still exists.
                auto meshPtr = registry.try_get<Components::Mesh>(entity);
                if (!meshPtr) {
                    ImGui::TreePop();
                    return;
                }
                auto& mesh = *meshPtr;

                auto currentMeshIndex = std::find_if(
                                            availableMeshResourcesIdsAndNames.begin(),
                                            availableMeshResourcesIdsAndNames.end(),
                                            [&mesh](const auto& meshIdAndName) { return meshIdAndName.first == mesh.resourceId; }) -
                                        availableMeshResourcesIdsAndNames.begin();
                std::string previewLabel = [&]() {
                    if (currentMeshIndex < 0 || currentMeshIndex >= availableMeshResourcesIdsAndNames.size()) {
                        return std::string("None");
                    }
                    return availableMeshResourcesIdsAndNames[currentMeshIndex].second;
                }();

                if (ImGui::BeginCombo("##MeshResource", previewLabel.c_str())) {
                    for (const auto& [meshId, meshName] : availableMeshResourcesIdsAndNames) {
                        const bool isSelected = (meshId == mesh.resourceId);

                        if (ImGui::Selectable(meshName.c_str(), isSelected)) {
                            mesh.resourceId = meshId;
                        }

                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }

                ImGui::TreePop();
            }
        }
        void renderStaticRigidBodyComponent(entt::registry& registry, entt::entity entity)
        {
            if (!registry.all_of<Components::StaticRigidBody>(entity))
                return;

            auto isOpened = ImGui::TreeNode("Static Rigid body");

            ImGui::OpenPopupOnItemClick("##StaticRigidBodyContext", ImGuiPopupFlags_MouseButtonRight);
            if (ImGui::BeginPopup("##StaticRigidBodyContext")) {
                if (ImGui::MenuItem("Remove static rigid body")) {
                    registry.remove<Components::StaticRigidBody>(entity);
                }
                ImGui::EndPopup();
            }

            if (isOpened) {
                // Because user could delete static rigid body component, we need to check if it still exists.
                auto staticRigidBodyPtr = registry.try_get<Components::StaticRigidBody>(entity);
                if (!staticRigidBodyPtr) {
                    ImGui::TreePop();
                    return;
                }

                ImGui::TreePop();
            }
        }

        void renderDynamicRigidBodyComponent(entt::registry& registry, entt::entity entity)
        {
            if (!registry.all_of<Components::DynamicRigidBody>(entity))
                return;

            auto isOpened = ImGui::TreeNode("Dynamic Rigid body");

            ImGui::OpenPopupOnItemClick("##DynamicRigidBodyContext", ImGuiPopupFlags_MouseButtonRight);
            if (ImGui::BeginPopup("##DynamicRigidBodyContext")) {
                if (ImGui::MenuItem("Remove dynamic rigid body")) {
                    registry.remove<Components::DynamicRigidBody>(entity);
                }
                ImGui::EndPopup();
            }

            if (isOpened) {
                // Because user could delete dynamic rigid body component, we need to check if it still exists.
                auto dynamicRigidBodyPtr = registry.try_get<Components::DynamicRigidBody>(entity);
                if (!dynamicRigidBodyPtr) {
                    ImGui::TreePop();
                    return;
                }

                ImGui::TreePop();
            }
        }

        void renderAABBComponent(entt::registry& registry, entt::entity entity)
        {
            if (!registry.all_of<Components::AABB>(entity))
                return;

            auto isOpened = ImGui::TreeNode("AABB");

            ImGui::OpenPopupOnItemClick("##AABBContext", ImGuiPopupFlags_MouseButtonRight);
            if (ImGui::BeginPopup("##AABBContext")) {
                if (ImGui::MenuItem("Remove aabb")) {
                    registry.remove<Components::AABB>(entity);
                }
                ImGui::EndPopup();
            }

            if (isOpened) {
                // Because user could delete aabb component, we need to check if it still exists.
                auto aabbPtr = registry.try_get<Components::AABB>(entity);
                if (!aabbPtr) {
                    ImGui::TreePop();
                    return;
                }
                auto& aabbLower = aabbPtr->lower;
                auto& aabbUpper = aabbPtr->upper;

                if (ImGui::BeginTable("AABBData", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("X");
                    ImGui::TableSetupColumn("Y");
                    ImGui::TableSetupColumn("Z");
                    ImGui::TableHeadersRow();

                    ImGui::TableNextRow();

                    // Lower
                    for (int i = 0; i < 3; i++) {
                        ImGui::TableSetColumnIndex(i);
                        std::string label = "##L_" + std::to_string(i);
                        ImGui::InputFloat(label.c_str(), &aabbLower[i], 0.1f, 1.0f, "%.3f");
                    }

                    ImGui::TableNextRow();

                    // Upper
                    for (int i = 0; i < 3; i++) {
                        ImGui::TableSetColumnIndex(i);
                        std::string label = "##M_" + std::to_string(i);
                        ImGui::InputFloat(label.c_str(), &aabbUpper[i], 0.1f, 1.0f, "%.3f");
                    }

                    ImGui::EndTable();
                }

                ImGui::TreePop();
            }
        }

        void renderNodeContextMenu(entt::registry& registry, entt::entity entity)
        {
            auto& name = registry.get<Components::Name>(entity);

            ImGui::OpenPopupOnItemClick(std::format("{}_popup", static_cast<uint32_t>(entity)).c_str(), ImGuiPopupFlags_MouseButtonRight);

            if (ImGui::BeginPopup(std::format("{}_popup", static_cast<uint32_t>(entity)).c_str())) {
                if (ImGui::BeginMenu("Add new component")) {
                    if (ImGui::MenuItem("Transform")) {
                        registry.emplace_or_replace<Components::Transform>(entity);
                    }
                    if (ImGui::MenuItem("Mesh")) {
                        registry.emplace_or_replace<Components::Mesh>(entity);
                    }
                    if (ImGui::BeginMenu("Rigid body")) {
                        // We can't have both bodies.
                        if (ImGui::MenuItem("Dynamic")) {
                            if (registry.all_of<Components::StaticRigidBody>(entity)) {
                                registry.remove<Components::StaticRigidBody>(entity);
                            }
                            registry.emplace_or_replace<Components::DynamicRigidBody>(entity);
                        }
                        if (ImGui::MenuItem("Static")) {
                            if (registry.all_of<Components::DynamicRigidBody>(entity)) {
                                registry.remove<Components::DynamicRigidBody>(entity);
                            }
                            registry.emplace_or_replace<Components::StaticRigidBody>(entity);
                        }

                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }

                // Note: This must be the last item in the popup!
                if (ImGui::MenuItem("Remove entity")) {
                    registry.destroy(entity);
                }

                ImGui::EndPopup();
            }
        }

        bool renderNode(entt::registry& registry, entt::entity entity)
        {
            auto& name = registry.get<Components::Name>(entity);

            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (registry.all_of<Components::Tags::SelectedNode>(entity)) {
                nodeFlags |= ImGuiTreeNodeFlags_Selected;
            }

            bool isOpened = ImGui::TreeNodeEx((void*)(intptr_t)entity, nodeFlags, "%s", name.name.c_str());

            if (ImGui::IsItemClicked()) {
                auto selectedNodeView = registry.view<Components::Tags::SelectedNode>();
                if (!selectedNodeView.empty()) {
                    registry.remove<Components::Tags::SelectedNode>(selectedNodeView.front());
                }
                registry.emplace_or_replace<Components::Tags::SelectedNode>(entity);
            }

            return isOpened;
        }

        bool renderRenamingNode(entt::registry& registry, entt::entity entity, std::string& newNameBuffer)
        {
            constexpr auto MAXIMUM_NAME_LENGTH = 256;

            newNameBuffer.resize(MAXIMUM_NAME_LENGTH);

            auto& name = registry.get<Components::Name>(entity);

            ImGui::SetKeyboardFocusHere();

            bool confirmRename = false;

            if (ImGui::InputText(
                    "##rename", newNameBuffer.data(), newNameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                confirmRename |= true; // confirm on Enter
            }
            if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                confirmRename |= true; // confirm on click outside
            }

            if (confirmRename) {
                name.name = newNameBuffer.data();
            }

            return confirmRename;
        }

        void renderAddingNewEntity(entt::registry& registry)
        {
            if (ImGui::BeginPopupContextWindow("##SceneHierarchyContext_AddingNewEntity", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
                if (ImGui::MenuItem("Create new entity")) {
                    auto newEntity = registry.create();
                    registry.emplace<Components::Name>(newEntity, "New Entity");
                }
                ImGui::EndPopup();
            }
        }
    } // namespace

    SceneHierarchyUI::SceneHierarchyUI(Resources::ContextResources& contextResources) : _contextResources(contextResources) {};

    void SceneHierarchyUI::Update(float deltaTime, Resources::Scene& scene)
    {
        ImGui::Begin("Scene Hierarchy");

        auto& registry    = scene.GetRegistry();
        auto& meshStorage = scene.GetMeshStorage();

        // So it is not done per entity.
        std::vector<std::reference_wrapper<Resources::MeshResource>> availableMeshResources;
        for (auto it = meshStorage.begin<Resources::MeshResource>(); it != meshStorage.end<Resources::MeshResource>(); ++it) {
            availableMeshResources.push_back((*it).second);
        }

        auto nameView = registry.view<Components::Name>();

        // We will display only entities that have a Name component.
        for (auto entity : nameView) {
            auto& name = registry.get<Components::Name>(entity);

            if (_renamingEntityData && _renamingEntityData->renamingEntity == entity) {
                bool confirmRename = renderRenamingNode(registry, entity, _renamingEntityData->newNameBuffer);

                if (confirmRename) {
                    _renamingEntityData.reset();
                }

                continue;
            }

            bool isOpened = renderNode(registry, entity);

            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) {
                _renamingEntityData = RenamingEntityData{entity, std::string(name.name)};
            }

            renderNodeContextMenu(registry, entity);

            if (isOpened) {
                renderTransformComponent(registry, entity);
                renderMeshComponent(availableMeshResources, registry, entity);
                renderStaticRigidBodyComponent(registry, entity);
                renderDynamicRigidBodyComponent(registry, entity);
                renderAABBComponent(registry, entity);

                ImGui::TreePop();
            }
        }

        renderAddingNewEntity(registry);

        ImGui::End();
    }
} // namespace Prism::UI