#include "systems/subsystems/physics_system/aabb_creating_subsystem.hpp"

namespace Prism::Systems::Subsystems::PhysicsSystem
{
    AABBCreatingSubsystem::AABBCreatingSubsystem(Resources::ContextResources& contextResources) : _contextResources(contextResources) {};

    void AABBCreatingSubsystem::Update(float deltaTime, Resources::Scene& scene) {}
} // namespace Prism::Systems::Subsystems::PhysicsSystem