#include "loaders/glad_loader.hpp"


#include <GLFW/glfw3.h>

#include <stdexcept>

namespace Prism::Loaders {
    GladLoader::result_type GladLoader::operator()() const {
        // if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        //     throw std::runtime_error("Failed to initialize GLAD");
        // }
    }
}; // namespace Prism::Loaders