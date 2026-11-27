#pragma once

#include "resources/context_resources.hpp"
#include "resources/scene.hpp"

namespace Prism::Systems::Subsystems::PhysicsSystem
{
    class GravityApplyingSubsystem
    {
    public:
        GravityApplyingSubsystem(Resources::ContextResources& contextResources);
        ~GravityApplyingSubsystem() = default;

        GravityApplyingSubsystem(GravityApplyingSubsystem& other)            = delete;
        GravityApplyingSubsystem& operator=(GravityApplyingSubsystem& other) = delete;

        GravityApplyingSubsystem(GravityApplyingSubsystem&& other)            = delete;
        GravityApplyingSubsystem& operator=(GravityApplyingSubsystem&& other) = delete;

        void Update(float deltaTime, Resources::Scene& scene);

    private:
        Resources::ContextResources& _contextResources;
    };
}; // namespace Prism::Systems::Subsystems::PhysicsSystem