#pragma once

#include "resources/resource.hpp"

#include <vector>

#include "volk/volk.h"

namespace Prism::Resources
{
    struct CPURaytraceResultResource : ResourceImpl<CPURaytraceResultResource>
    {
        struct PerPixelData
        {
            constexpr static auto PIXEL_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;

            uint8_t aabbIntersections = 0;
            uint8_t triangleHits      = 0;
            uint8_t b                 = 0;
            uint8_t a                 = 0;
        };

        struct Statistics
        {
            uint32_t averageAABBIntersections;
            uint32_t averageTriangleHits;
        };

        CPURaytraceResultResource(std::vector<PerPixelData> data, int width, int height);
        ~CPURaytraceResultResource() = default;

        CPURaytraceResultResource(const CPURaytraceResultResource& other)            = delete;
        CPURaytraceResultResource& operator=(const CPURaytraceResultResource& other) = delete;

        CPURaytraceResultResource(CPURaytraceResultResource&& other) noexcept;
        CPURaytraceResultResource& operator=(CPURaytraceResultResource&& other) noexcept;

        PerPixelData* Data() { return _data.data(); }
        int           GetWidth() const { return _width; }
        int           GetHeight() const { return _height; }
        Statistics    GetStatistics();

    private:
        friend void swap(CPURaytraceResultResource& a, CPURaytraceResultResource& b) noexcept;

        std::vector<PerPixelData> _data       = {};
        int                       _width      = 0;
        int                       _height     = 0;
        std::optional<Statistics> _statistics = {};
    };
} // namespace Prism::Resources