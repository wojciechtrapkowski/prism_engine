#pragma once


#include <GLFW/glfw3.h>

#include "resources/context_resources.hpp"
#include "resources/scene.hpp"

namespace Prism::Systems {
    class CameraCreationSystem {
      public:
        CameraCreationSystem(Resources::ContextResources &contextResources);
        ~CameraCreationSystem() = default;

        CameraCreationSystem(CameraCreationSystem &other) = delete;
        CameraCreationSystem &operator=(CameraCreationSystem &other) = delete;

        CameraCreationSystem(CameraCreationSystem &&other) = delete;
        CameraCreationSystem &operator=(CameraCreationSystem &&other) = delete;

        void Initialize();

        void Update(float deltaTime, Resources::Scene &scene);

      private:
        Resources::ContextResources &m_contextResources;
    };
}; // namespace Prism::Systems