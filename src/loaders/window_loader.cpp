#include "loaders/window_loader.hpp"

#include "vulkan/vulkan.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

namespace Prism::Loaders {
    namespace /* anonymous */ {
        const int SCR_WIDTH = 1200;
        const int SCR_HEIGHT = 600;
    } // namespace

    WindowLoader::result_type WindowLoader::operator()() const {

        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW!");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        auto window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PrismEngine", nullptr, nullptr);

        if (!window) {
            throw std::runtime_error("Failed to create GLFW window");
            glfwTerminate();
        }

        // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        Resources::WindowResource windowResource{std::move(window)};

        windowResource.UpdateWindowExtent(SCR_WIDTH, SCR_HEIGHT);

        return windowResource;
    }
}; // namespace Prism::Loaders