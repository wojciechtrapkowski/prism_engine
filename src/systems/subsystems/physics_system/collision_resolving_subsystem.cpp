#include "systems/subsystems/physics_system/collision_resolving_subsystem.hpp"

namespace Prism::Systems::Subsystems::PhysicsSystem
{
    CollisionResolvingSubsystem::CollisionResolvingSubsystem(Resources::ContextResources& contextResources) : _contextResources(contextResources) {};

    void CollisionResolvingSubsystem::Update(float deltaTime, Resources::Scene& scene) {}
} // namespace Prism::Systems::Subsystems::PhysicsSystem