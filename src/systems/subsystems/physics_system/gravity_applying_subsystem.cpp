#include "systems/subsystems/physics_system/gravity_applying_subsystem.hpp"

#include "components/rigid_body.hpp"
#include "components/transform.hpp"

namespace Prism::Systems::Subsystems::PhysicsSystem
{
    namespace
    {
        constexpr auto GRAVITY_CONSTANT = 10.0f;

    }
    GravityApplyingSubsystem::GravityApplyingSubsystem(Resources::ContextResources& contextResources) : _contextResources(contextResources) {};

    void GravityApplyingSubsystem::Update(float deltaTime, Resources::Scene& scene)
    {
        auto& registry = scene.GetRegistry();

        auto dynamicRigidBodyView = registry.view<Components::DynamicRigidBody, Components::Transform>();

        for (auto&& [entity, dynamicRigidBody, transform] : dynamicRigidBodyView.each()) {
            glm::vec4& position = transform.transform[3];

            // s = vo * t + 1/2 * a * t^2
            // vo = 0
            // removed 1/2 and t^2, so it is a bit faster
            position.y -= deltaTime * GRAVITY_CONSTANT;
        }
    }
} // namespace Prism::Systems::Subsystems::PhysicsSystem