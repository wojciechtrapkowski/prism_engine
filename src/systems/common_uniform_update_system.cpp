#include "systems/common_uniform_update_system.hpp"

#include "components/camera.hpp"
#include "components/fps_camera_control.hpp"
#include "components/tags.hpp"
#include "components/transform.hpp"

#include "resources/common_resource.hpp"
#include "resources/vulkan/vk_buffer_resource.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace Prism::Systems {
    namespace {} // namespace

    CommonUniformUpdateSystem::CommonUniformUpdateSystem(Resources::ContextResources &contextResources) : m_contextResources(contextResources) {};

    void CommonUniformUpdateSystem::Initialize() {

    };

    void CommonUniformUpdateSystem::Update(float deltaTime, Resources::Scene &scene) {
        auto window = m_contextResources.GetWindowResource().GetWindow();

        auto &registry = scene.GetRegistry();

        auto activeCameraView = registry.view<Components::Tags::ActiveCamera>();
        if (activeCameraView.empty()) {
            return;
        }
        auto cameraEntity = activeCameraView.front();

        if (!registry.all_of<Components::Camera, Components::Transform>(cameraEntity)) {
            return;
        }

        auto &camera = registry.get<Components::Camera>(cameraEntity);
        auto &transform = registry.get<Components::Transform>(cameraEntity);

        Resources::CommonResource shaderData{};
        shaderData.view = camera.view;
        shaderData.projection = glm::scale(camera.projection, glm::vec3(1.0f, -1.0f, 1.0f)); // Flip Y in the projection matrix - VULKAN
        shaderData.cameraPosition = glm::vec4(transform.transform[3]);

        auto &vulkanResource = m_contextResources.GetVulkanResource();
        auto &resourceStorage = m_contextResources.GetResourceStorage();
        auto currentFrame = vulkanResource.GetCurrentFrameOffset();
        auto uniformBufferOpt =
            resourceStorage.Get<Resources::VkBufferResource<Resources::CommonResource>>(Resources::CommonResource::UNIFORM_BUFFER_ID, currentFrame);
        if (!uniformBufferOpt) {
            auto uniformBuffer = std::make_unique<Resources::VkBufferResource<Resources::CommonResource>>(
                vulkanResource.GetVmaAllocator(), sizeof(Resources::CommonResource), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            resourceStorage.Insert<Resources::VkBufferResource<Resources::CommonResource>>(Resources::CommonResource::UNIFORM_BUFFER_ID,
                                                                                           std::move(uniformBuffer), currentFrame);
            uniformBufferOpt =
                resourceStorage.Get<Resources::VkBufferResource<Resources::CommonResource>>(Resources::CommonResource::UNIFORM_BUFFER_ID, currentFrame);
        }
        auto &uniformBuffer = uniformBufferOpt->get();

        void *data = nullptr;
        VmaAllocator allocator = vulkanResource.GetVmaAllocator();
        VmaAllocation allocation = uniformBuffer.GetAllocation();

        if (vmaMapMemory(allocator, allocation, &data) == VK_SUCCESS) {
            std::memcpy(data, &shaderData, uniformBuffer.GetBufferSize());
            vmaUnmapMemory(allocator, allocation);
        }
    };
} // namespace Prism::Systems