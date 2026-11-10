#pragma once

namespace Prism::Components
{
    struct MeshDrawingSystemSettings
    {
        enum class MeshDrawingMode
        {
            RASTERIZATION,
            CUSTOM_RAYTRACING,
            API_RAYTRACING // It is optional, so it must be at the very end!
        } drawingMode = MeshDrawingMode::RASTERIZATION;

        struct
        {
            int selectedCPURaytraceResult = 0;
            int raytracingWidth           = 512;
            int raytracingHeight          = 256;            
        } customRaytracedGeometryDrawingSystemSettings;
    };

    struct AABBDrawingSystemSettings
    {
        bool disable = false;
    };
} // namespace Prism::Components