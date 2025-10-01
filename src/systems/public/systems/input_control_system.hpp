#pragma once


#include <GLFW/glfw3.h>

#include "resources/context_resources.hpp"

namespace Prism::Systems {
    struct InputControlSystem {
      public:
        InputControlSystem(Resources::ContextResources &contextResources);
        ~InputControlSystem() = default;

        InputControlSystem(InputControlSystem &other) = delete;
        InputControlSystem &operator=(InputControlSystem &other) = delete;

        InputControlSystem(InputControlSystem &&other) = delete;
        InputControlSystem &operator=(InputControlSystem &&other) = delete;

        void Initialize();

        void Update(float deltaTime);

      private:
        Resources::ContextResources &m_contextResources;
    };
}; // namespace Prism::Systems