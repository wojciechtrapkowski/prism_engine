#pragma once

#include <utility>

namespace Prism::Events {
    struct WindowCloseEvent {};

    struct WindowResizeEvent {
        int newWidth;
        int newHeight;
    };
}; // namespace Prism::Events