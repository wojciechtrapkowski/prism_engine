#pragma once

namespace Prism::Components
{
    struct MeshDrawingSystemSettings
    {
        enum class MeshDrawingMode
        {
            RASTERIZATION,
            API_RAYTRACING,
            CUSTOM_RAYTRACING
        } drawingMode = MeshDrawingMode::RASTERIZATION;
    };
} // namespace Prism::Components