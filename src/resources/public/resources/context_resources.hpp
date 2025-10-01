#pragma once

#include "resources/resource.hpp"

#include "resources/imgui_resource.hpp"
#include "resources/resource_storage.hpp"
#include "resources/vulkan_resource.hpp"
#include "resources/window_resource.hpp"

#include <entt/entt.hpp>

namespace Prism::Resources {
    struct ContextResources : ResourceImpl<ContextResources> {
        ContextResources(Resources::WindowResource &&windowResource, Resources::VulkanResource &&vulkanResource, Resources::ImGuiResource &&imguiResource);
        ~ContextResources() = default;

        ContextResources(ContextResources &other) = delete;
        ContextResources &operator=(ContextResources &other) = delete;

        ContextResources(ContextResources &&other) = default;
        ContextResources &operator=(ContextResources &&other) = default;

        entt::dispatcher &GetDispatcher() { return dispatcher; }

        Resources::WindowResource &GetWindowResource() { return windowResource; }

        Resources::VulkanResource &GetVulkanResource() { return vulkanResource; }

        Resources::ImGuiResource &GetImGuiResource() { return imguiResource; }

        Resources::ResourceStorage &GetResourceStorage() { return resourceStorage; }

      private:
        entt::dispatcher dispatcher;
        Resources::WindowResource windowResource;
        Resources::VulkanResource vulkanResource;
        Resources::ImGuiResource imguiResource;
        Resources::ResourceStorage resourceStorage;
    };
}; // namespace Prism::Resources