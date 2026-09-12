#include "resources/mesh_resource.hpp"

namespace Prism::Resources
{
    MeshResource::MeshResource(
        std::string                                 name,
        Resources::VkBufferResource<Vertex>         vertexBuffer,
        Resources::VkBufferResource<Index>          indexBuffer,
        std::vector<Vertex>                         vertices,
        std::vector<Index>                          indices,
        std::optional<Resources::VkTextureResource> texture) :
        _name(std::move(name)), _vertexBuffer(std::move(vertexBuffer)), _indexBuffer(std::move(indexBuffer)), _vertices(std::move(vertices)),
        _indices(std::move(indices)), _texture(std::move(texture))
    {}

    MeshResource::MeshResource(MeshResource&& other)
    {
        swap(*this, other);
    }

    MeshResource& MeshResource::operator=(MeshResource&& other)
    {
        swap(*this, other);
        return *this;
    }

    void swap(MeshResource& lhs, MeshResource& rhs) noexcept
    {
        using std::swap;
        swap(lhs._name, rhs._name);
        swap(lhs._vertexBuffer, rhs._vertexBuffer);
        swap(lhs._indexBuffer, rhs._indexBuffer);
        swap(lhs._vertices, rhs._vertices);
        swap(lhs._indices, rhs._indices);
        swap(lhs._texture, rhs._texture);
    }
} // namespace Prism::Resources