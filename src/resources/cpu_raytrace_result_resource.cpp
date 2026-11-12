#include "resources/cpu_raytrace_result_resource.hpp"

namespace Prism::Resources
{
    CPURaytraceResultResource::CPURaytraceResultResource(std::vector<PerPixelData> data, int width, int height) : _data(data), _width(width), _height(height){};

    CPURaytraceResultResource::CPURaytraceResultResource(CPURaytraceResultResource&& other) noexcept
    {
        swap(*this, other);
    }

    CPURaytraceResultResource& CPURaytraceResultResource::operator=(CPURaytraceResultResource&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    void swap(CPURaytraceResultResource& a, CPURaytraceResultResource& b) noexcept
    {
        using std::swap;

        swap(a._data, b._data);
        swap(a._width, b._width);
        swap(a._height, b._height);
    }
} // namespace Prism::Resources