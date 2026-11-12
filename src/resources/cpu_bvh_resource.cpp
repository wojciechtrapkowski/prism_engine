#include "resources/cpu_bvh_resource.hpp"

namespace Prism::Resources
{
    CPUBVHResource::CPUBVHResource(std::vector<BVHNode> nodes) : _nodes(nodes){};

    CPUBVHResource::CPUBVHResource(CPUBVHResource&& other) noexcept
    {
        swap(*this, other);
    }

    CPUBVHResource& CPUBVHResource::operator=(CPUBVHResource&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    void swap(CPUBVHResource& a, CPUBVHResource& b) noexcept
    {
        using std::swap;

        swap(a._nodes, b._nodes);
    }
} // namespace Prism::Resources