#pragma once

#include "resources/context_resources.hpp"
#include "resources/scene.hpp"

#include "events/raytrace_events.hpp"

#include "imgui.h"

namespace Prism::UI
{
    class SystemsSettingsUI
    {
    public:
        SystemsSettingsUI(Resources::ContextResources& contextResources);
        ~SystemsSettingsUI() = default;

        SystemsSettingsUI(const SystemsSettingsUI&)            = delete;
        SystemsSettingsUI& operator=(const SystemsSettingsUI&) = delete;

        SystemsSettingsUI(SystemsSettingsUI&&)            = delete;
        SystemsSettingsUI& operator=(SystemsSettingsUI&&) = delete;

        void Update(float deltaTime, Resources::Scene& scene);

    private:
        Resources::ContextResources& _contextResources;

        entt::scoped_connection    _onCpuRaytraceFinishedScopedConnection;
        std::optional<Events::CPURaytraceFinishedEvent> _cpuRaytraceFinishedOpt;
        std::vector<Events::CPURaytraceFinishedEvent>   _cpuRaytraceResults;

        bool _isTraceOpen = false;

        void onCPURaytraceFinishEvent(Events::CPURaytraceFinishedEvent& event);
    };
} // namespace Prism::UI