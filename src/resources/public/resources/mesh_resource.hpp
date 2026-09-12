#pragma once

#include <glm/glm.hpp>

#include "volk/volk.h"

#include <vector>

#include "resources/resource.hpp"
#include "resources/vulkan/vk_buffer_resource.hpp"
#include "resources/vulkan/vk_texture_resource.hpp"

namespace Prism::Resources
{
    struct MeshResource : ResourceImpl<MeshResource>
    {
        static inline const VkFormat    VERTEX_TYPE = VK_FORMAT_R32G32B32_SFLOAT;
        static inline const VkIndexType INDEX_TYPE  = VK_INDEX_TYPE_UINT32;

        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 textureUV;
        };

        struct Index
        {
            uint32_t idx;
        };

        MeshResource(
            std::string                                 name,
            Resources::VkBufferResource<Vertex>         vertexBuffer,
            Resources::VkBufferResource<Index>          indexBuffer,
            std::vector<Vertex>                         vertices,
            std::vector<Index>                          indices,
            std::optional<Resources::VkTextureResource> texture = std::nullopt);

        ~MeshResource() = default;

        MeshResource(MeshResource& other)            = delete;
        MeshResource& operator=(MeshResource& other) = delete;

        MeshResource(MeshResource&& other);
        MeshResource& operator=(MeshResource&& other);

        Resources::VkBufferResource<Vertex>& GetVertexBuffer() { return _vertexBuffer; }

        Resources::VkBufferResource<Index>& GetIndexBuffer() { return _indexBuffer; }

        std::optional<Resources::VkTextureResource>& GetTexture() { return _texture; }

        std::vector<Vertex>& GetVertices() { return _vertices; }

        const std::string& GetName() const { return _name; }

        VkFormat    GetVertexType() const { return VERTEX_TYPE; }
        VkIndexType GetIndexType() const { return INDEX_TYPE; }

        friend void swap(MeshResource& lhs, MeshResource& rhs) noexcept;

    private:
        std::string _name = "None";

        Resources::VkBufferResource<Vertex> _vertexBuffer = {};
        Resources::VkBufferResource<Index>  _indexBuffer  = {};

        std::vector<Vertex> _vertices = {};
        std::vector<Index>  _indices  = {};

        std::optional<Resources::VkTextureResource> _texture = std::nullopt;
    };

}; // namespace Prism::Resources