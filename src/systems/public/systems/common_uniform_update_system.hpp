#pragma once


#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "resources/context_resources.hpp"
#include "resources/scene.hpp"

namespace Prism::Systems {
    class CommonUniformUpdateSystem {
      public:
        CommonUniformUpdateSystem(Resources::ContextResources &contextResources);

        ~CommonUniformUpdateSystem() = default;

        CommonUniformUpdateSystem(CommonUniformUpdateSystem &other) = delete;
        CommonUniformUpdateSystem &operator=(CommonUniformUpdateSystem &other) = delete;

        CommonUniformUpdateSystem(CommonUniformUpdateSystem &&other) = delete;
        CommonUniformUpdateSystem &operator=(CommonUniformUpdateSystem &&other) = delete;

        void Initialize();

        void Update(float deltaTime, Resources::Scene &scene);

      private:
        Resources::ContextResources &m_contextResources;
    };
}; // namespace Prism::Systems