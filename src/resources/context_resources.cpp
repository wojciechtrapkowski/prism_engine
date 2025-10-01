#include "resources/context_resources.hpp"

namespace Prism::Resources {

    ContextResources::ContextResources(Resources::WindowResource &&windowResource, Resources::VulkanResource &&vulkanResource,
                                       Resources::ImGuiResource &&imguiResource)
        : dispatcher{}, windowResource(std::move(windowResource)), vulkanResource(std::move(vulkanResource)), imguiResource(std::move(imguiResource)),
          resourceStorage{} {}

} // namespace Prism::Resources