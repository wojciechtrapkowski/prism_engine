#pragma once

#include "resources/context_resources.hpp"
#include "resources/scene.hpp"

namespace Prism::Systems::Subsystems::PhysicsSystem
{
    class CollisionResolvingSubsystem
    {
    public:
        CollisionResolvingSubsystem(Resources::ContextResources& contextResources);
        ~CollisionResolvingSubsystem() = default;

        CollisionResolvingSubsystem(CollisionResolvingSubsystem& other)            = delete;
        CollisionResolvingSubsystem& operator=(CollisionResolvingSubsystem& other) = delete;

        CollisionResolvingSubsystem(CollisionResolvingSubsystem&& other)            = delete;
        CollisionResolvingSubsystem& operator=(CollisionResolvingSubsystem&& other) = delete;

        void Update(float deltaTime, Resources::Scene& scene);

    private:
        Resources::ContextResources& _contextResources;
    };
}; // namespace Prism::Systems::Subsystems::PhysicsSystem