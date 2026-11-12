#pragma once

#include "resources/vulkan_resource.hpp"
#include "resources/scene.hpp"
#include "resources/context_resources.hpp"
#include "resources/render_target_resource.hpp"
#include "resources/vulkan/vk_staging_buffer_resource.hpp"
#include "resources/vulkan/vk_acceleration_structure_resource.hpp"

#include "components/transform.hpp"

#include "events/raytrace_events.hpp"

#include "volk/volk.h"
#include "entt/signal/sigh.hpp"

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

        inline static const std::string_view CPU_RAYTRACE_RESULTS_BUFFER_ID_PREFIX = "CustomRaytracedGeometryDrawingSubsystem/CPURaytraceResultsBuffer";

        inline static const std::string_view MESH_BVH_BUFFER_ID_PREFIX = "CustomRaytracedGeometryDrawingSubsystem/BVHBuffer";

        void Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::VkStagingBufferResource& stagingBuffer);

        void Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget);

    private:
        Resources::ContextResources& _contextResources;

        size_t                                       _cpuRaytraceCounter = 0;
        entt::scoped_connection                      _onCpuRaytraceStartEventConnection;
        std::optional<Events::CPURaytraceStartEvent> _cpuRaytraceStartEventToProcessOpt = std::nullopt;

        void onCPURaytraceStartEvent(Events::CPURaytraceStartEvent& event);
    };
}; // namespace Prism::Systems::Subsystems::MeshDrawingSystem