#include "context/context.hpp"

#include "loaders/glad_loader.hpp"
#include "loaders/imgui_loader.hpp"
#include "loaders/mesh_loader.hpp"
#include "loaders/vulkan_loader.hpp"
#include "loaders/window_loader.hpp"

#include "systems/camera_creation_system.hpp"
#include "systems/common_uniform_update_system.hpp"
#include "systems/event_poll_system.hpp"
#include "systems/fps_motion_control_system.hpp"
#include "systems/input_control_system.hpp"
#include "systems/window_resize_system.hpp"

#include "managers/scene_draw_systems_manager.hpp"
#include "managers/scene_update_systems_manager.hpp"

#include "resources/context_resources.hpp"
#include "resources/scene.hpp"


#include <format>
#include <iostream>

namespace Prism::Context {
    namespace {
        struct FPSCounter {
            size_t frames = 0;
            double lastTime = glfwGetTime();
        };

        Resources::ContextResources createContextResources() {
            Loaders::WindowLoader windowLoader;
            auto windowResource = windowLoader();

            Loaders::VulkanLoader vulkanLoader;
            auto vulkanResource = vulkanLoader(windowResource);

            if (!vulkanResource) {
                throw std::runtime_error("Couldn't load Vulkan!");
            }

            Loaders::ImGuiLoader imGuiLoader;
            auto imguiResource = imGuiLoader(windowResource, vulkanResource.value());

            if (!imguiResource) {
                throw std::runtime_error("Couldn't load imgui!");
            }

            return {std::move(windowResource), std::move(*vulkanResource), std::move(*imguiResource)};
        }
    } // namespace

    Context::Context() : m_contextResources{createContextResources()} {
        m_windowCloseEventConnection = m_contextResources.GetDispatcher().sink<Events::WindowCloseEvent>().connect<&Context::onWindowClose>(this);
    }

    void Context::RunEngine() {
        Systems::EventPollSystem eventPollSystem{m_contextResources};

        Systems::InputControlSystem inputControlSystem{m_contextResources};

        Systems::WindowResizeSystem windowResizeSystem{m_contextResources};

        Managers::SceneDrawSystemsManager sceneDrawSystemsManager{m_contextResources};

        Managers::SceneUpdateSystemsManager sceneUpdateSystemsManager{m_contextResources};

        Resources::Scene scene{};

        Loaders::MeshLoader meshLoader{};

        Resources::VkStagingBufferResource stagingBuffer{m_contextResources.GetVulkanResource().GetVmaAllocator()};

        auto backpackModelOpt = meshLoader(m_contextResources.GetVulkanResource(), stagingBuffer, "backpack.obj");
        if (!backpackModelOpt) {
            std::cerr << "Couldn't load backpack model!" << std::endl;
        } else {
            auto &backpackModel = *backpackModelOpt;
            auto backpackId = std::hash<const char *>{}("MeshResources/Backpack");
            scene.AddNewMesh(backpackId, "Backpack", std::move(backpackModel));

            std::cout << "Loaded backpack model!" << std::endl;
        }

        // auto cubeModelOpt = meshLoader("cube.obj");
        // if (!cubeModelOpt) {
        //     std::cerr << "Couldn't load cube model!" << std::endl;
        // } else {
        //     auto &cubeModel = *cubeModelOpt;
        //     auto cubeId = std::hash<const char *>{}("MeshResources/Cube");
        //     scene.AddNewMesh(cubeId, "Cube", std::move(cubeModel));
        // }

        eventPollSystem.Initialize();

        inputControlSystem.Initialize();

        sceneUpdateSystemsManager.Initialize();

        sceneDrawSystemsManager.Initialize();

        float deltaTime = 0.0f;
        float lastFrameTime = 0.0f;

        FPSCounter fpsCounter{};

        // Scope for cleanup
        {
            auto &windowResource = m_contextResources.GetWindowResource();
            auto &vulkanResource = m_contextResources.GetVulkanResource();

            while (m_isRunning) {
                float currentTime = glfwGetTime();
                deltaTime = currentTime - lastFrameTime;
                lastFrameTime = currentTime;

                inputControlSystem.Update(deltaTime);

                eventPollSystem.Update(deltaTime);

                windowResizeSystem.Update(deltaTime);

                sceneUpdateSystemsManager.Update(deltaTime, scene);

                sceneDrawSystemsManager.Update(deltaTime, scene, stagingBuffer);

                // Display FPS counter every second
                fpsCounter.frames++;
                if (currentTime - fpsCounter.lastTime >= 1.0) {
                    std::string title = std::format("FPS: {}", fpsCounter.frames);
                    glfwSetWindowTitle(windowResource.GetWindow(), title.c_str());
                    fpsCounter.frames = 0;
                    fpsCounter.lastTime = currentTime;
                }
            }

            vkDeviceWaitIdle(m_contextResources.GetVulkanResource().GetDevice());
        }
    }

    void Context::onWindowClose(Events::WindowCloseEvent &event) { m_isRunning = false; }
}; // namespace Prism::Context