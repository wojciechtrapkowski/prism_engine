#pragma once

#include "resources/vulkan_resource.hpp"
#include "resources/scene.hpp"
#include "resources/context_resources.hpp"
#include "resources/render_target_resource.hpp"
#include "resources/vulkan/vk_staging_buffer_resource.hpp"
#include "resources/vulkan/vk_acceleration_structure_resource.hpp"

#include "components/transform.hpp"

#include "volk/volk.h"

#include <vector>

namespace Prism::Systems::Subsystems::MeshDrawingSystem
{
    struct CustomRaytracedGeometryDrawingSubsystem
    {
    public:
        CustomRaytracedGeometryDrawingSubsystem(Resources::ContextResources& contextResources);
        ~CustomRaytracedGeometryDrawingSubsystem();

        CustomRaytracedGeometryDrawingSubsystem(CustomRaytracedGeometryDrawingSubsystem& other)            = delete;
        CustomRaytracedGeometryDrawingSubsystem& operator=(CustomRaytracedGeometryDrawingSubsystem& other) = delete;

        CustomRaytracedGeometryDrawingSubsystem(CustomRaytracedGeometryDrawingSubsystem&& other)            = delete;
        CustomRaytracedGeometryDrawingSubsystem& operator=(CustomRaytracedGeometryDrawingSubsystem&& other) = delete;

        inline static const Resources::Resource::ID BVH_BUFFER_ID = std::hash<std::string_view>{}("CustomRaytracedGeometryDrawingSubsystem/BVHBuffer");

        void Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::VkStagingBufferResource& stagingBuffer);

        void Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget);

    private:
        Resources::ContextResources& _contextResources;
    };
}; // namespace Prism::Systems::Subsystems::MeshDrawingSystem