#pragma once

#include "resources/context_resources.hpp"
#include "resources/scene.hpp"

#include <utility>

namespace Prism::Systems
{
    namespace Subsystems::PhysicsSystem
    {
        class GravityApplyingSubsystem;
        class AABBCreatingSubsystem;
        class CollisionResolvingSubsystem;
    } // namespace Subsystems::PhysicsSystem
    class PhysicsSystem
    {
    public:
        PhysicsSystem(Resources::ContextResources& contextResources);
        ~PhysicsSystem();

        PhysicsSystem(PhysicsSystem& other)            = delete;
        PhysicsSystem& operator=(PhysicsSystem& other) = delete;

        PhysicsSystem(PhysicsSystem&& other)            = delete;
        PhysicsSystem& operator=(PhysicsSystem&& other) = delete;

        void Update(float deltaTime, Resources::Scene& scene);

    private:
        Resources::ContextResources& _contextResources;

        std::unique_ptr<Subsystems::PhysicsSystem::GravityApplyingSubsystem>    _gravityApplyingSubsystem;
        std::unique_ptr<Subsystems::PhysicsSystem::AABBCreatingSubsystem>       _aabbCreatingSubsystem;
        std::unique_ptr<Subsystems::PhysicsSystem::CollisionResolvingSubsystem> _collisionResolvingSubsystem;
    };
} // namespace Prism::Systems