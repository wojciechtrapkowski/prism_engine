#include "managers/scene_update_systems_manager.hpp"

namespace Prism::Managers
{
    SceneUpdateSystemsManager::SceneUpdateSystemsManager(Resources::ContextResources& contextResources) :
        _cameraCreationSystem{contextResources}, _motionControlSystem{contextResources}, _commonUniformUpdateSystem{contextResources},
        _physicsSystem{contextResources}
    {}

    void SceneUpdateSystemsManager::Update(float deltaTime, Resources::Scene& scene)
    {
        _cameraCreationSystem.Update(deltaTime, scene);
        _motionControlSystem.Update(deltaTime, scene);
        _commonUniformUpdateSystem.Update(deltaTime, scene);
        _physicsSystem.Update(deltaTime, scene);
    }
} // namespace Prism::Managers