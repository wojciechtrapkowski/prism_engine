#include "managers/scene_update_systems_manager.hpp"

namespace Prism::Managers {
    SceneUpdateSystemsManager::SceneUpdateSystemsManager(Resources::ContextResources &contextResources)
        : cameraCreationSystem{contextResources}, fpsMotionControlSystem{contextResources}, commonUniformUpdateSystem{contextResources} {}

    void SceneUpdateSystemsManager::Initialize() {
        cameraCreationSystem.Initialize();
        fpsMotionControlSystem.Initialize();
        commonUniformUpdateSystem.Initialize();
    }

    void SceneUpdateSystemsManager::Update(float deltaTime, Resources::Scene &scene) {
        cameraCreationSystem.Update(deltaTime, scene);
        fpsMotionControlSystem.Update(deltaTime, scene);
        commonUniformUpdateSystem.Update(deltaTime, scene);
    }
} // namespace Prism::Managers