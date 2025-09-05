#pragma once

#include "resources/context_resources.hpp"
#include "resources/scene.hpp"

namespace Prism::UI {
    namespace {
        constexpr auto FPS_CAMERA_NAME = "FPS Camera";

        struct CameraType {
            const char *name;
            std::function<bool(entt::registry &, entt::entity)> checkIfActive;
            std::function<void(entt::registry &)> makeActive;
        };
    }; // namespace

    class CameraSettingsUI {
      public:
        CameraSettingsUI(Resources::ContextResources &contextResources);
        ~CameraSettingsUI() = default;

        CameraSettingsUI(const CameraSettingsUI &) = delete;
        CameraSettingsUI &operator=(const CameraSettingsUI &) = delete;

        CameraSettingsUI(CameraSettingsUI &&) = delete;
        CameraSettingsUI &operator=(CameraSettingsUI &&) = delete;

        void Update(float deltaTime, Resources::Scene &scene);

      private:
        Resources::ContextResources &m_contextResources;
    };
} // namespace Prism::UI