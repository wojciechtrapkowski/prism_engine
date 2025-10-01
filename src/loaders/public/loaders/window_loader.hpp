#pragma once

#include "resources/window_resource.hpp"

#include <memory>

#include <GLFW/glfw3.h>

namespace Prism::Loaders {
    struct WindowLoader {
        using result_type = Resources::WindowResource;

        WindowLoader() = default;
        ~WindowLoader() = default;

        WindowLoader(WindowLoader &&other) = default;
        WindowLoader &operator=(WindowLoader &&) = default;

        WindowLoader(WindowLoader &other) = delete;
        WindowLoader &operator=(WindowLoader &) = delete;

        result_type operator()() const;
    };
}; // namespace Prism::Loaders