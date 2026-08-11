#include "systems/subsystems/physics_system/gravity_applying_subsystem.hpp"

namespace Prism::Systems::Subsystems::PhysicsSystem
{
    GravityApplyingSubsystem::GravityApplyingSubsystem(Resources::ContextResources& contextResources) : _contextResources(contextResources) {};

    void GravityApplyingSubsystem::Update(float deltaTime, Resources::Scene& scene) {}
} // namespace Prism::Systems::Subsystems::PhysicsSystem