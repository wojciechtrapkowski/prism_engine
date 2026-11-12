#pragma once

#include "resources/context_resources.hpp"
#include "resources/render_target_resource.hpp"
#include "resources/scene.hpp"
#include "resources/vulkan/vk_staging_buffer_resource.hpp"

namespace Prism::Systems
{
    namespace Subsystems::MeshDrawingSystem
    {
        class RasterizedGeometryDrawingSubsystem;
        class APIRaytracedGeometryDrawingSubsystem;
        class CustomRaytracedGeometryDrawingSubsystem;
    } // namespace Subsystems::MeshDrawingSystem
    class MeshDrawingSystem
    {
    public:
        MeshDrawingSystem(Resources::ContextResources& contextResources);
        ~MeshDrawingSystem();

        MeshDrawingSystem(MeshDrawingSystem& other)            = delete;
        MeshDrawingSystem& operator=(MeshDrawingSystem& other) = delete;

        MeshDrawingSystem(MeshDrawingSystem&& other)            = delete;
        MeshDrawingSystem& operator=(MeshDrawingSystem&& other) = delete;

        void Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::VkStagingBufferResource& stagingBuffer, Resources::Scene& scene);

        void Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget);

    private:
        Resources::ContextResources& _contextResources;

        std::unique_ptr<Subsystems::MeshDrawingSystem::RasterizedGeometryDrawingSubsystem>      _rasterizedGeometryDrawingSubsystem;
        std::unique_ptr<Subsystems::MeshDrawingSystem::APIRaytracedGeometryDrawingSubsystem>    _apiRaytracedGeometryDrawingSubsystem;
        std::unique_ptr<Subsystems::MeshDrawingSystem::CustomRaytracedGeometryDrawingSubsystem> _customRaytracedGeometryDrawingSubsystem;
    };
}; // namespace Prism::Systems