#include "systems/subsystems/physics_system/aabb_creating_subsystem.hpp"

#include "components/mesh.hpp"
#include "components/aabb.hpp"

#include "resources/mesh_resource.hpp"

namespace Prism::Systems::Subsystems::PhysicsSystem
{
    AABBCreatingSubsystem::AABBCreatingSubsystem(Resources::ContextResources& contextResources) : _contextResources(contextResources) {};

    void AABBCreatingSubsystem::Update(float deltaTime, Resources::Scene& scene)
    {
        auto& registry = scene.GetRegistry();

        auto& meshStorage = scene.GetMeshStorage();

        auto meshView = registry.view<Components::Mesh>(entt::exclude<Components::AABB>);

        for (auto&& [entity, meshComponent] : meshView.each()) {
            auto meshResourceOpt = meshStorage.Get<Resources::MeshResource>(meshComponent.resourceId);

            if (!meshResourceOpt) {
                continue;
            }

            auto& meshResource = *meshResourceOpt;

            auto numericLimitsMin = std::numeric_limits<float>::min();
            auto numericLimitsMax = std::numeric_limits<float>::max();

            glm::vec4 aabbLower{numericLimitsMax};
            glm::vec4 aabbUpper{numericLimitsMin};

            for (const auto& vertex : meshResource.get().GetVertices()) {
                aabbLower.x = std::min(aabbLower.x, vertex.position.x);
                aabbLower.y = std::min(aabbLower.y, vertex.position.y);
                aabbLower.z = std::min(aabbLower.z, vertex.position.z);

                aabbUpper.x = std::max(aabbUpper.x, vertex.position.x);
                aabbUpper.y = std::max(aabbUpper.y, vertex.position.y);
                aabbUpper.z = std::max(aabbUpper.z, vertex.position.z);
            }

            registry.emplace<Components::AABB>(entity, aabbLower, aabbUpper);
        }
    }
} // namespace Prism::Systems::Subsystems::PhysicsSystem