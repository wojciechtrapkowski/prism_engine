#include "systems/subsystems/physics_system/aabb_creating_subsystem.hpp"

#include "components/mesh.hpp"
#include "components/aabb.hpp"
#include "components/transform.hpp"

#include "resources/mesh_resource.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_decompose.hpp"

namespace Prism::Systems::Subsystems::PhysicsSystem
{
    AABBCreatingSubsystem::AABBCreatingSubsystem(Resources::ContextResources& contextResources) : _contextResources(contextResources) {};

    void AABBCreatingSubsystem::Update(float deltaTime, Resources::Scene& scene)
    {
        auto& registry = scene.GetRegistry();

        auto& meshStorage = scene.GetMeshStorage();

        // Update existing AABBs
        auto aabbView = registry.view<Components::AABB, Components::Transform>();
        for (auto&& [entity, aabbComponent, transformComponent] : aabbView.each()) {
            auto& transform = transformComponent.transform;

            // Make sure we don't multiply by rotation.
            auto scale       = glm::vec3{glm::length(transform[0]), glm::length(transform[1]), glm::length(transform[2])};
            auto translation = glm::vec3(transform[3]);

            glm::mat4 transformWithoutRotation{1.0f};
            transformWithoutRotation = glm::translate(transformWithoutRotation, translation);
            transformWithoutRotation = glm::scale(transformWithoutRotation, scale);

            auto& lower = aabbComponent.lower;
            auto& upper = aabbComponent.upper;

            lower = transformWithoutRotation * aabbComponent.originalLower;
            upper = transformWithoutRotation * aabbComponent.originalUpper;
        }

        // Create new AABBs
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
                aabbLower.w = 1.0f;

                aabbUpper.x = std::max(aabbUpper.x, vertex.position.x);
                aabbUpper.y = std::max(aabbUpper.y, vertex.position.y);
                aabbUpper.z = std::max(aabbUpper.z, vertex.position.z);
                aabbUpper.w = 1.0f;
            }

            registry.emplace<Components::AABB>(entity, aabbLower, aabbUpper, aabbLower, aabbUpper);
        }
    }
} // namespace Prism::Systems::Subsystems::PhysicsSystem