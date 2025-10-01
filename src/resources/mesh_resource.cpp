#include "resources/mesh_resource.hpp"

namespace Prism::Resources {
    MeshResource::MeshResource(Resources::VkBufferResource<Vertex> vertexBuffer, Resources::VkBufferResource<Index> indexBuffer)
        : vertexBuffer(std::move(vertexBuffer)), indexBuffer(std::move(indexBuffer)) {}

    MeshResource::MeshResource(MeshResource &&other) {
        using std::swap;
        swap(*this, other);
    }

    MeshResource &MeshResource::operator=(MeshResource &&other) {
        using std::swap;
        swap(*this, other);
        return *this;
    }

    void swap(MeshResource &lhs, MeshResource &rhs) noexcept {
        using std::swap;
        swap(lhs.vertexBuffer, rhs.vertexBuffer);
        swap(lhs.indexBuffer, rhs.indexBuffer);
    }
} // namespace Prism::Resources