#include "loaders/window_loader.hpp"

#include "utils/opengl_debug.hpp"

#include "glad/glad.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

namespace Prism::Loaders {
    namespace /* anonymous */ {
        const unsigned int SCR_WIDTH = 800;
        const unsigned int SCR_HEIGHT = 600;

        void framebufferSizeCallback(GLFWwindow *window, int width,
                                     int height) {
            GLCheck(glViewport(0, 0, width, height));
        }

    } // namespace

    WindowLoader::result_type WindowLoader::operator()() const {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        auto window =
            std::unique_ptr<GLFWwindow, GLFWwindowDeleter>(glfwCreateWindow(
                SCR_WIDTH, SCR_HEIGHT, "PrismEngine", nullptr, nullptr));

        if (!window.get()) {
            throw std::runtime_error("Failed to create GLFW window");
            glfwTerminate();
        }

        glfwMakeContextCurrent(window.get());
        glfwSetFramebufferSizeCallback(window.get(), framebufferSizeCallback);
        glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        return window;
    }
}; // namespace Prism::Loaders