#pragma once

#include <cstdint>

#include "resources/resource.hpp"

#include "volk/volk.h"

namespace Prism::Events
{
    struct CPURaytraceStartEvent
    {};

    struct CPURaytraceFinishedEvent
    {
        VkDescriptorSet textureId;
        int             width;
        int             height;
    };
} // namespace Prism::Events