#pragma once

#include <array>
#include <glm/glm.hpp>

namespace Prism::Assets
{
    struct Box
    {
        static constexpr std::array<glm::vec4, 8> VERTICES = {
            {{1.0f, 1.0f, -1.0f, 0.0f},
             {1.0f, -1.0f, -1.0f, 0.0f},
             {1.0f, 1.0f, 1.0f, 0.0f},
             {1.0f, -1.0f, 1.0f, 0.0f},
             {-1.0f, 1.0f, -1.0f, 0.0f},
             {-1.0f, -1.0f, -1.0f, 0.0f},
             {-1.0f, 1.0f, 1.0f, 0.0f},
             {-1.0f, -1.0f, 1.0f, 0.0f}}};

        static constexpr std::array<uint32_t, 36> INDICES = {
            {0, 2, 3, 0, 3, 1, 4, 5, 7, 4, 7, 6, 4, 6, 2, 4, 2, 0, 5, 1, 3, 5, 3, 7, 2, 6, 7, 2, 7, 3, 0, 1, 5, 0, 5, 4}};
    };
} // namespace Prism::Assets