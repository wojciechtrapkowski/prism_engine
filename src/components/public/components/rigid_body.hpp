#pragma once

namespace Prism::Components
{
    struct RigidBody
    {
        virtual ~RigidBody() = default;

    protected:
        // A trick so we can't put pure rigid body on an entity.

        RigidBody() = default;
    };

    struct StaticRigidBody : RigidBody
    {
        ~StaticRigidBody() override = default;
    };

    struct DynamicRigidBody : RigidBody
    {
        ~DynamicRigidBody() override = default;
    };
}; // namespace Prism::Components