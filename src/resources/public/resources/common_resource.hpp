#pragma once

#include "resources/resource.hpp"

#include <glm/glm.hpp>


#include <GLFW/glfw3.h>

namespace Prism::Resources {

    struct CommonResource {
        glm::mat4 view{};
        glm::mat4 projection{};
        glm::vec4 cameraPosition{};

        inline static const Resources::Resource::ID UNIFORM_BUFFER_ID = std::hash<std::string_view>{}("CommonUniformUpdateSystem/UniformBufferResource");
    };
}; // namespace Prism::Resources