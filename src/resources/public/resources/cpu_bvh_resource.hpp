#pragma once

#include "resources/resource.hpp"

#include <optional>
#include <vector>

#include "entt/entt.hpp"
#include "glm/glm.hpp"

namespace Prism::Resources
{
    struct CPUBVHResource : ResourceImpl<CPUBVHResource>
    {
        // This should be transformed so it supports instances as well as quad leaves.
        struct BVHLeaf
        {
            uint32_t triangleId;
            glm::vec3 v0;
            glm::vec3 v1;
            glm::vec3 v2;
        };

        struct BVHNode
        {
            struct
            {
                glm::vec4 lower;
                glm::vec4 upper;
            } aabb;

            std::vector<uint32_t> children; // always positive offsets, from this node to children.
            uint32_t              numChildren;

            std::optional<BVHLeaf> leaf;
        };

        CPUBVHResource(std::vector<BVHNode> nodes);
        ~CPUBVHResource() = default;

        CPUBVHResource(const CPUBVHResource& other)            = delete;
        CPUBVHResource& operator=(const CPUBVHResource& other) = delete;

        CPUBVHResource(CPUBVHResource&& other) noexcept;
        CPUBVHResource& operator=(CPUBVHResource&& other) noexcept;

        const std::vector<BVHNode>& GetNodes() const { return _nodes; };

    private:
        friend void swap(CPUBVHResource& a, CPUBVHResource& b) noexcept;

        std::vector<BVHNode> _nodes = {};
    };
} // namespace Prism::Resources