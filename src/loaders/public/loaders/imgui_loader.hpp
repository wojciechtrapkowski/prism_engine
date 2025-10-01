#pragma once

#include <GLFW/glfw3.h>

#include <optional>
#include <utility>

#include "resources/imgui_resource.hpp"
#include "resources/vulkan_resource.hpp"
#include "resources/window_resource.hpp"

namespace Prism::Loaders {
    struct ImGuiLoader {
        using result_type = std::optional<Resources::ImGuiResource>;

        ImGuiLoader() = default;
        ~ImGuiLoader() = default;

        ImGuiLoader(ImGuiLoader &&other) = default;
        ImGuiLoader &operator=(ImGuiLoader &&) = default;

        ImGuiLoader(ImGuiLoader &other) = delete;
        ImGuiLoader &operator=(ImGuiLoader &) = delete;

        result_type operator()(Resources::WindowResource &windowResource, Resources::VulkanResource &vulkanResource) const;
    };
}; // namespace Prism::Loaders