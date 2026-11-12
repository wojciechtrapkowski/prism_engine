#include "systems/subsystems/mesh_drawing_system/custom_raytraced_geometry_drawing_subsystem.hpp"

#include "components/aabb.hpp"
#include "components/mesh.hpp"
#include "components/camera.hpp"
#include "components/transform.hpp"
#include "components/tags.hpp"
#include "components/systems_settings.hpp"

#include "resources/resource_storage.hpp"
#include "resources/mesh_resource.hpp"

#include "loaders/cpu_bvh_build_morton_loader.hpp"
#include "loaders/cpu_bvh_build_sah_loader.hpp"
#include "loaders/cpu_raytrace_result_loader.hpp"

#include <optional>
#include <stack>
#include <format>
#include <string>

#include "glm/glm.hpp"
#include "imgui_impl_vulkan.h"

namespace Prism::Systems::Subsystems::MeshDrawingSystem
{
    namespace
    {} // namespace

    CustomRaytracedGeometryDrawingSubsystem::CustomRaytracedGeometryDrawingSubsystem(Resources::ContextResources& contextResources) :
        _contextResources(contextResources)
    {
        _onCpuRaytraceStartEventConnection =
            _contextResources.GetDispatcher().sink<Events::CPURaytraceStartEvent>().connect<&CustomRaytracedGeometryDrawingSubsystem::onCPURaytraceStartEvent>(
                this);
    };

    CustomRaytracedGeometryDrawingSubsystem::~CustomRaytracedGeometryDrawingSubsystem() {}

    void CustomRaytracedGeometryDrawingSubsystem::Update(
        float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::VkStagingBufferResource& stagingBuffer)
    {
        constexpr auto EACH_AABB_IS_ENTITY = true;

        auto& vulkanResource = _contextResources.GetVulkanResource();
        auto& registry       = scene.GetRegistry();
        auto& systemsStorage = scene.GetSystemsStorage();
        auto& meshStorage    = scene.GetMeshStorage();

        Components::MeshDrawingSystemSettings meshDrawingSystemSettings{};
        auto                                  meshDrawingSystemSettingsView = registry.view<Components::MeshDrawingSystemSettings>();
        if (!meshDrawingSystemSettingsView.empty()) {
            meshDrawingSystemSettings = registry.get<Components::MeshDrawingSystemSettings>(meshDrawingSystemSettingsView.front());
        }

        auto currentlySelectedBuilder = meshDrawingSystemSettings.customRaytracedGeometryDrawingSystemSettings.builder;

        auto getMeshBVHBufferId = [currentlySelectedBuilder](auto& resourceId) {
            return std::hash<std::string_view>{}(std::format("{}/{}/{}", MESH_BVH_BUFFER_ID_PREFIX, static_cast<int>(currentlySelectedBuilder), std::to_string(resourceId)));
        };

        Loaders::CPUBVHBuildMortonLoader buildBVHMortonLoader{};
        Loaders::CPUBVHBuildSAHLoader    buildBVHSAHLoader{};
        auto                             meshView = registry.view<Components::Mesh>();
        for (auto&& [entity, meshComponent] : meshView.each()) {
            auto id = getMeshBVHBufferId(meshComponent.resourceId);

            if (!systemsStorage.Contains(id)) {
                if (!meshStorage.Contains(meshComponent.resourceId)) {
                    continue;
                }
                auto& mesh = meshStorage.Get<Resources::MeshResource>(meshComponent.resourceId)->get();

                // At this point, mesh will contain AABB calculated from physics system, if not, we should calculate it.
                if (!registry.all_of<Components::AABB>(entity)) {
                    auto numericLimitsMin = std::numeric_limits<float>::lowest();
                    auto numericLimitsMax = std::numeric_limits<float>::max();

                    glm::vec4 aabbLower{numericLimitsMax};
                    glm::vec4 aabbUpper{numericLimitsMin};

                    for (const auto& vertex : mesh.GetVertices()) {
                        aabbLower.x = std::min(aabbLower.x, vertex.position.x);
                        aabbLower.y = std::min(aabbLower.y, vertex.position.y);
                        aabbLower.z = std::min(aabbLower.z, vertex.position.z);
                        aabbLower.w = 1.0f;

                        aabbUpper.x = std::max(aabbUpper.x, vertex.position.x);
                        aabbUpper.y = std::max(aabbUpper.y, vertex.position.y);
                        aabbUpper.z = std::max(aabbUpper.z, vertex.position.z);
                        aabbUpper.w = 1.0f;
                    }

                    registry.emplace<Components::AABB>(entity, aabbLower, aabbUpper, aabbLower, aabbUpper);
                }
                Components::AABB& sceneAABB = registry.get<Components::AABB>(entity);

                std::optional<Resources::CPUBVHResource> bvhResourceOpt;
                if (currentlySelectedBuilder == Components::MeshDrawingSystemSettings::CustomRaytracedGeometryDrawingSystemSettings::Builder::MORTON) {
                    bvhResourceOpt = buildBVHMortonLoader(mesh, sceneAABB.lower, sceneAABB.upper);
                } else if (currentlySelectedBuilder == Components::MeshDrawingSystemSettings::CustomRaytracedGeometryDrawingSystemSettings::Builder::SAH) {
                    bvhResourceOpt = buildBVHSAHLoader(mesh, sceneAABB.lower, sceneAABB.upper);
                }

                if (!bvhResourceOpt) {
                    throw std::runtime_error("Couldn't build BVH!");
                }
                auto& bvhResource = bvhResourceOpt.value();

                systemsStorage.Insert(id, std::make_unique<Resources::CPUBVHResource>(std::move(bvhResource)));
            }
        }

        auto activeCameraView = registry.view<Components::Tags::ActiveCamera>();

        std::vector<std::reference_wrapper<Resources::CPUBVHResource>> bvhsToTraverse;
        std::vector<glm::mat4>                                         transforms;
        for (auto&& [entity, meshComponent] : meshView.each()) {
            auto id     = getMeshBVHBufferId(meshComponent.resourceId);
            auto bvhOpt = systemsStorage.Get<Resources::CPUBVHResource>(id);
            if (bvhOpt) {
                bvhsToTraverse.push_back(bvhOpt.value());
                if (registry.all_of<Components::Transform>(entity)) {
                    transforms.push_back(registry.get<Components::Transform>(entity).transform);
                } else {
                    transforms.emplace_back(1.0f);
                }
            }
        }

        bool runCPURaytrace = true;
        runCPURaytrace &= _cpuRaytraceStartEventToProcessOpt.has_value();
        runCPURaytrace &= !activeCameraView.empty();
        runCPURaytrace &= !bvhsToTraverse.empty();

        auto cameraEntity = activeCameraView.front();

        runCPURaytrace &= registry.all_of<Components::Camera, Components::Transform>(cameraEntity);

        if (runCPURaytrace) {
            auto height = meshDrawingSystemSettings.customRaytracedGeometryDrawingSystemSettings.raytracingHeight;
            auto width  = meshDrawingSystemSettings.customRaytracedGeometryDrawingSystemSettings.raytracingWidth;

            auto& cameraComponent    = registry.get<Components::Camera>(cameraEntity);
            auto& transformComponent = registry.get<Components::Transform>(cameraEntity);

            Loaders::CPURaytraceResultLoader                      raytraceResultLoader{};
            Loaders::CPURaytraceResultLoader::CPURaytraceSettings raytraceSettings;

            raytraceSettings.cameraRelated.position   = glm::vec3(transformComponent.transform[3]);
            raytraceSettings.cameraRelated.projection = cameraComponent.projection;
            raytraceSettings.cameraRelated.view       = cameraComponent.view;
            raytraceSettings.height                   = height;
            raytraceSettings.width                    = width;

            auto raytraceResultOpt = raytraceResultLoader(raytraceSettings, bvhsToTraverse, transforms);
            if (!raytraceResultOpt) {
                throw std::runtime_error("Couldn't raytrace!");
            }
            auto& raytraceResult = raytraceResultOpt.value();

            const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * sizeof(Resources::CPURaytraceResultResource::PerPixelData)); // 4 bytes.

            VkImageCreateInfo imageInfo{};
            imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType     = VK_IMAGE_TYPE_2D;
            imageInfo.extent        = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
            imageInfo.mipLevels     = 1;
            imageInfo.arrayLayers   = 1;
            imageInfo.format        = Resources::CPURaytraceResultResource::PerPixelData::PIXEL_FORMAT;
            imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

            VmaAllocationCreateInfo imageAllocInfo{};
            imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VkImage       textureImage      = VK_NULL_HANDLE;
            VmaAllocation textureAllocation = VK_NULL_HANDLE;
            if (vmaCreateImage(vulkanResource.GetVmaAllocator(), &imageInfo, &imageAllocInfo, &textureImage, &textureAllocation, nullptr) != VK_SUCCESS) {
                throw std::runtime_error("Error: couldn't create image from raytracing result.");
            }

            Resources::VkImageResource imageResource(vulkanResource.GetVmaAllocator(), textureAllocation, textureImage, width, height);

            VkImageMemoryBarrier barrierToTransfer{};
            barrierToTransfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrierToTransfer.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
            barrierToTransfer.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrierToTransfer.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrierToTransfer.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrierToTransfer.image                           = imageResource.GetImage();
            barrierToTransfer.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrierToTransfer.subresourceRange.baseMipLevel   = 0;
            barrierToTransfer.subresourceRange.levelCount     = 1;
            barrierToTransfer.subresourceRange.baseArrayLayer = 0;
            barrierToTransfer.subresourceRange.layerCount     = 1;
            barrierToTransfer.srcAccessMask                   = 0;
            barrierToTransfer.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(
                commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierToTransfer);

            stagingBuffer.CopyToImage(imageResource.GetImage(), raytraceResult.Data(), imageSize, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image            = imageResource.GetImage();
            viewInfo.viewType         = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format           = Resources::CPURaytraceResultResource::PerPixelData::PIXEL_FORMAT;
            viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            VkImageView imageView = VK_NULL_HANDLE;
            if (vkCreateImageView(vulkanResource.GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                throw std::runtime_error("Error: couldn't create image view from raytracing result.");
            }

            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter               = VK_FILTER_LINEAR;
            samplerInfo.minFilter               = VK_FILTER_LINEAR;
            samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable        = VK_FALSE;
            samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable           = VK_FALSE;
            samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.minLod                  = 0.0f;
            samplerInfo.maxLod                  = 0.0f;

            VkSampler sampler = VK_NULL_HANDLE;
            if (vkCreateSampler(vulkanResource.GetDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
                vkDestroyImageView(vulkanResource.GetDevice(), imageView, nullptr);
                throw std::runtime_error("Error: couldn't create image sampler from raytracing result.");
            }

            auto textureResource = Resources::VkTextureResource(vulkanResource.GetDevice(), std::move(imageResource), imageView, sampler);

            Resources::Resource::ID textureId =
                std::hash<std::string_view>{}(std::format("{}_{}", CPU_RAYTRACE_RESULTS_BUFFER_ID_PREFIX, _cpuRaytraceCounter++));

            systemsStorage.Insert(textureId, std::make_unique<Resources::VkTextureResource>(std::move(textureResource)));
        }
    }

    void CustomRaytracedGeometryDrawingSubsystem::Render(
        float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget)
    {
        auto& systemsStorage = scene.GetSystemsStorage();

        Resources::Resource::ID textureId = std::hash<std::string_view>{}(std::format("{}_{}", CPU_RAYTRACE_RESULTS_BUFFER_ID_PREFIX, _cpuRaytraceCounter - 1));
        auto                    textureResourceOpt = systemsStorage.Get<Resources::VkTextureResource>(textureId);
        if (_cpuRaytraceStartEventToProcessOpt && textureResourceOpt) {
            auto& textureResource = textureResourceOpt->get();

            auto imguiTextureId =
                ImGui_ImplVulkan_AddTexture(textureResource.GetSampler(), textureResource.GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            _contextResources.GetDispatcher().enqueue<Events::CPURaytraceFinishedEvent>(
                imguiTextureId, textureResource.GetWidth(), textureResource.GetHeight());
        }
        _cpuRaytraceStartEventToProcessOpt = std::nullopt;
    }

    void CustomRaytracedGeometryDrawingSubsystem::onCPURaytraceStartEvent(Events::CPURaytraceStartEvent& event)
    {
        _cpuRaytraceStartEventToProcessOpt = event;
    }
} // namespace Prism::Systems::Subsystems::MeshDrawingSystem