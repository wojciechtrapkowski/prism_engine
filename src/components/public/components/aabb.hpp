#pragma once

#include "glm/glm.hpp"

namespace Prism::Components
{
    struct AABB
    {
        glm::vec4 originalLower;
        glm::vec4 originalUpper;

        glm::vec4 lower;
        glm::vec4 upper;
    };
} // namespace Prism::Components