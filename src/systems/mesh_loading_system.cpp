#include "systems/mesh_loading_system.hpp"

#include "loaders/mesh_loader.hpp"

// Testing purposes
#include "components/transform.hpp"
#include "components/mesh.hpp"
#include "components/name.hpp"
#include "components/tags.hpp"

#include <iostream>

namespace Prism::Systems
{
    namespace
    {} // namespace

    MeshLoadingSystem::MeshLoadingSystem(Resources::ContextResources& contextResources) : _contextResources(contextResources)
    {
        auto& dispatcher = _contextResources.GetDispatcher();

        _onMeshFileOpenConnection = dispatcher.sink<Events::MeshFileOpenEvent>().connect<&MeshLoadingSystem::onMeshFileOpen>(this);
    };

    void MeshLoadingSystem::Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::VkStagingBufferResource& stagingBuffer, Resources::Scene& scene)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        auto& registry = scene.GetRegistry();

        // For testing purposes.
        bool static firstFrames = true;
        if (firstFrames && _contextResources.GetVulkanResource().GetCurrentFrameOffset() == 0) {
            Loaders::MeshLoader meshLoader;
            auto backpackModelOpt = meshLoader(_contextResources.GetVulkanResource(), commandBuffer, stagingBuffer, "models/backpack-texture/scene.gltf");
            if (!backpackModelOpt) {
                std::cerr << "Couldn't load backpack model!" << std::endl;
            } else {
                auto& backpackModel = *backpackModelOpt;
                auto  backpackId    = std::hash<std::string>{}("MeshResources/Backpack");

                auto& meshStorage = scene.GetMeshStorage();
                meshStorage.Insert<Resources::MeshResource>(backpackId, std::move(backpackModel));
            }

            auto cubeModelOpt = meshLoader(_contextResources.GetVulkanResource(), commandBuffer, stagingBuffer, "models/backpack.obj");
            if (!cubeModelOpt) {
                std::cerr << "Couldn't load cube model!" << std::endl;
            } else {
                auto& cubeModel = *cubeModelOpt;
                auto  cubeId    = std::hash<std::string>{}("MeshResources/Cube");

                auto& meshStorage = scene.GetMeshStorage();
                meshStorage.Insert<Resources::MeshResource>(cubeId, std::move(cubeModel));
            }
        }
        if (firstFrames && _contextResources.GetVulkanResource().GetCurrentFrameOffset() == 1) {
            auto& registry = scene.GetRegistry();

            {
                auto entity     = registry.create();
                auto backpackId = std::hash<std::string>{}("MeshResources/Backpack");
                registry.emplace<Components::Mesh>(entity, backpackId);
                registry.emplace<Components::Transform>(entity, glm::mat4(1.0f));
                registry.emplace<Components::Name>(entity, "Backpack");
                registry.emplace<Components::Tags::ActivePlayer>(entity);
            }

            {
                auto cubeId = std::hash<std::string>{}("MeshResources/Cube");
                auto entity = registry.create();
                registry.emplace<Components::Mesh>(entity, cubeId);
                registry.emplace<Components::Transform>(entity, glm::mat4(1.0f));
                registry.emplace<Components::Name>(entity, "Cube");
            }
            firstFrames = false;
        }

        if (_meshFilePathToLoad) {
            if (std::find(_meshesLoaded.begin(), _meshesLoaded.end(), *_meshFilePathToLoad) != _meshesLoaded.end()) {
                vkEndCommandBuffer(commandBuffer);
                return;
            }

            Loaders::MeshLoader meshLoader{};

            auto meshResource = meshLoader(_contextResources.GetVulkanResource(), commandBuffer, stagingBuffer, *_meshFilePathToLoad);
            if (!meshResource) {
#ifdef DEBUG
                std::cerr << "Couldn't load mesh from file: " << *_meshFilePathToLoad << std::endl;
#endif
                return;
            }

            // This should be done when we go back to the first frame frames in flight count.
            auto& meshStorage = scene.GetMeshStorage();
            auto  meshId      = std::hash<std::string>{}("MeshResources/" + *_meshFilePathToLoad);

            if (!meshStorage.Contains(meshId)) {
                meshStorage.Insert<Resources::MeshResource>(meshId, std::move(*meshResource));
            }

            _meshesLoaded.push_back(*_meshFilePathToLoad);

            _meshFilePathToLoad = std::nullopt;
        }

        vkEndCommandBuffer(commandBuffer);
    }

    void MeshLoadingSystem::onMeshFileOpen(const Events::MeshFileOpenEvent& event)
    {
        _meshFilePathToLoad = event.filePath;
    }

} // namespace Prism::Systems
