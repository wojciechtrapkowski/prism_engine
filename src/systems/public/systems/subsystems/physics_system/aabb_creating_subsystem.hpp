#pragma once

#include "resources/context_resources.hpp"
#include "resources/scene.hpp"

namespace Prism::Systems::Subsystems::PhysicsSystem
{
    class AABBCreatingSubsystem
    {
    public:
        AABBCreatingSubsystem(Resources::ContextResources& contextResources);
        ~AABBCreatingSubsystem() = default;

        AABBCreatingSubsystem(AABBCreatingSubsystem& other)            = delete;
        AABBCreatingSubsystem& operator=(AABBCreatingSubsystem& other) = delete;

        AABBCreatingSubsystem(AABBCreatingSubsystem&& other)            = delete;
        AABBCreatingSubsystem& operator=(AABBCreatingSubsystem&& other) = delete;

        void Update(float deltaTime, Resources::Scene& scene);

    private:
        Resources::ContextResources& _contextResources;
    };
}; // namespace Prism::Systems::Subsystems::PhysicsSystem