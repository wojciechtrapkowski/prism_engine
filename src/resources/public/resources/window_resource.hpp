#pragma once

#include "resources/resource.hpp"
#include "vulkan/vulkan.h"

#include "GLFW/glfw3.h"

#include <utility>

namespace Prism::Resources {
    struct WindowResource : ResourceImpl<WindowResource> {
        WindowResource(GLFWwindow *window);
        ~WindowResource();

        WindowResource(WindowResource &other) = delete;
        WindowResource &operator=(WindowResource &other) = delete;

        WindowResource(WindowResource &&other) noexcept;
        WindowResource &operator=(WindowResource &&other) noexcept;

        GLFWwindow *GetWindow() const { return window; }

        std::pair<int, int> GetWindowExtent() { return std::make_pair(currentWidth, currentHeight); };

        void UpdateWindowExtent(int width, int height) {
            currentWidth = width;
            currentHeight = height;
        }

      private:
        friend void swap(WindowResource &lhs, WindowResource &rhs) noexcept;

        GLFWwindow *window = nullptr;

        int currentWidth = 0;
        int currentHeight = 0;
    };
}; // namespace Prism::Resources