#include "systems/physics_system.hpp"

#include "systems/subsystems/physics_system/gravity_applying_subsystem.hpp"
#include "systems/subsystems/physics_system/aabb_creating_subsystem.hpp"
#include "systems/subsystems/physics_system/collision_resolving_subsystem.hpp"

namespace Prism::Systems
{
    PhysicsSystem::PhysicsSystem(Resources::ContextResources& contextResources) : _contextResources(contextResources)
    {
        _gravityApplyingSubsystem    = std::make_unique<Subsystems::PhysicsSystem::GravityApplyingSubsystem>(contextResources);
        _aabbCreatingSubsystem       = std::make_unique<Subsystems::PhysicsSystem::AABBCreatingSubsystem>(contextResources);
        _collisionResolvingSubsystem = std::make_unique<Subsystems::PhysicsSystem::CollisionResolvingSubsystem>(contextResources);
    };

    PhysicsSystem::~PhysicsSystem() {}

    void PhysicsSystem::Update(float deltaTime, Resources::Scene& scene)
    {
        _gravityApplyingSubsystem->Update(deltaTime, scene);
        _aabbCreatingSubsystem->Update(deltaTime, scene);
        _collisionResolvingSubsystem->Update(deltaTime, scene);
    }
} // namespace Prism::Systems