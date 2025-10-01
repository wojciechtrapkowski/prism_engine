#include "resources/window_resource.hpp"

namespace Prism::Resources {
    WindowResource::WindowResource(GLFWwindow *window) : window(window) {};

    WindowResource::~WindowResource() {
        if (window != nullptr) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }

    WindowResource::WindowResource(WindowResource &&other) noexcept { swap(*this, other); }

    WindowResource &WindowResource::operator=(WindowResource &&other) noexcept {
        swap(*this, other);
        return *this;
    }

    void swap(WindowResource &lhs, WindowResource &rhs) noexcept {
        using std::swap;
        swap(lhs.window, rhs.window);
        swap(lhs.currentWidth, rhs.currentWidth);
        swap(lhs.currentHeight, rhs.currentHeight);
    }
}; // namespace Prism::Resources

; // namespace Prism::Resources