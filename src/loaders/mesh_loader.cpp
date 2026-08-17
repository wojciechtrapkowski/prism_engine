#include "loaders/mesh_loader.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <filesystem>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Prism::Loaders
{
    namespace
    {
        constexpr unsigned int MODELS_LOADING_FLAGS = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices |
                                                      aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;

        using Vertex = Resources::MeshResource::Vertex;
        using Index  = Resources::MeshResource::Index;

        struct MeshDescriptor
        {
            std::vector<Vertex>        vertices;
            std::vector<Index>         indices;
            std::optional<std::string> texturePath;
        };

        glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& aiMat)
        {
            return glm::mat4(
                aiMat.a1,
                aiMat.b1,
                aiMat.c1,
                aiMat.d1,
                aiMat.a2,
                aiMat.b2,
                aiMat.c2,
                aiMat.d2,
                aiMat.a3,
                aiMat.b3,
                aiMat.c3,
                aiMat.d3,
                aiMat.a4,
                aiMat.b4,
                aiMat.c4,
                aiMat.d4);
        }

        void processNode(
            const aiNode*        node,
            const aiScene*       scene,
            glm::mat4            parentTransform,
            std::vector<Vertex>& vertices,
            std::vector<Index>&  indices,
            size_t&              vertexOffset)
        {
            glm::mat4 nodeTransform   = aiMatrix4x4ToGlm(node->mTransformation);
            glm::mat4 globalTransform = parentTransform * nodeTransform;

            for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

                for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                    const aiVector3D& pos = mesh->mVertices[v];

                    glm::vec4 transformedPos = globalTransform * glm::vec4(pos.x, pos.y, pos.z, 1.0f);

                    Vertex vert;
                    vert.position = {transformedPos.x, transformedPos.y, transformedPos.z};
                    vert.normal   = {mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z};

                    if (mesh->mTextureCoords[0]) {
                        vert.textureUV = {mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y};
                    } else {
                        vert.textureUV = {0.0f, 0.0f};
                    }

                    vertices.push_back(std::move(vert));
                }

                for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
                    const aiFace& face = mesh->mFaces[f];
                    for (unsigned int idx = 0; idx < face.mNumIndices; ++idx) {
                        Index index;
                        index.idx = face.mIndices[idx] + vertexOffset;
                        indices.push_back(std::move(index));
                    }
                }

                vertexOffset += mesh->mNumVertices;
            }

            for (unsigned int c = 0; c < node->mNumChildren; ++c) {
                processNode(node->mChildren[c], scene, globalTransform, vertices, indices, vertexOffset);
            }
        }

        std::optional<std::string> processTexture(const aiScene* scene)
        {
            if (scene->mNumMaterials == 0) {
                return std::nullopt;
            }

            for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
                if (scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                    aiString texturePath;
                    scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
                    return std::string(texturePath.C_Str());
                }
            }

            return std::nullopt;
        }

        std::optional<MeshDescriptor> loadModel(Assimp::Importer& importer, const std::string& path)
        {
            const aiScene* scene = importer.ReadFile(path, MODELS_LOADING_FLAGS);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
                return std::nullopt;
            }

            std::vector<Vertex> vertices     = {};
            std::vector<Index>  indices      = {};
            size_t              vertexOffset = 0;

            processNode(scene->mRootNode, scene, glm::mat4(1.0f), vertices, indices, vertexOffset);

            auto texturePath = processTexture(scene);

            return MeshDescriptor{.vertices = std::move(vertices), .indices = std::move(indices), .texturePath = std::move(texturePath)};
        }

        std::optional<Resources::VkTextureResource> loadTexture(
            Resources::VulkanResource&          vulkanResource,
            VkCommandBuffer                     commandBuffer,
            Resources::VkStagingBufferResource& stagingBuffer,
            const std::optional<std::string>&   pathOpt)
        {
            // TODO: Add resizing texture, so we can creatue texture arrays - one image with multiple layers.
            if (!pathOpt) {
                return std::nullopt;
            }
            auto& path = *pathOpt;

            int      width, height, channels;
            stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            if (!data) {
                std::cerr << "Failed to load texture: " << path << std::endl;
                return std::nullopt;
            }

            const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * 4); // RGBA = 4 bytes

            VkImageCreateInfo imageInfo{};
            imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType     = VK_IMAGE_TYPE_2D;
            imageInfo.extent        = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
            imageInfo.mipLevels     = 1;
            imageInfo.arrayLayers   = 1;
            imageInfo.format        = VK_FORMAT_R8G8B8A8_SRGB;
            imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

            VmaAllocationCreateInfo imageAllocInfo{};
            imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VkImage       textureImage      = VK_NULL_HANDLE;
            VmaAllocation textureAllocation = VK_NULL_HANDLE;
            if (vmaCreateImage(vulkanResource.GetVmaAllocator(), &imageInfo, &imageAllocInfo, &textureImage, &textureAllocation, nullptr) != VK_SUCCESS) {
                stbi_image_free(data);
                return std::nullopt;
            }

            Resources::VkImageResource imageResource(vulkanResource.GetVmaAllocator(), textureAllocation, textureImage);

            VkImageMemoryBarrier barrierToTransfer{};
            barrierToTransfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrierToTransfer.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
            barrierToTransfer.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrierToTransfer.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrierToTransfer.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrierToTransfer.image                           = imageResource.GetImage();
            barrierToTransfer.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrierToTransfer.subresourceRange.baseMipLevel   = 0;
            barrierToTransfer.subresourceRange.levelCount     = 1;
            barrierToTransfer.subresourceRange.baseArrayLayer = 0;
            barrierToTransfer.subresourceRange.layerCount     = 1;
            barrierToTransfer.srcAccessMask                   = 0;
            barrierToTransfer.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(
                commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierToTransfer);

            stagingBuffer.CopyToImage(imageResource.GetImage(), data, imageSize, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image            = imageResource.GetImage();
            viewInfo.viewType         = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format           = VK_FORMAT_R8G8B8A8_SRGB;
            viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            VkImageView imageView = VK_NULL_HANDLE;
            if (vkCreateImageView(vulkanResource.GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                return std::nullopt;
            }

            // TODO: Add support for mip mapping.
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter               = VK_FILTER_LINEAR;
            samplerInfo.minFilter               = VK_FILTER_LINEAR;
            samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable        = VK_FALSE;
            samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable           = VK_FALSE;
            samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.minLod                  = 0.0f;
            samplerInfo.maxLod                  = 0.0f;

            VkSampler sampler = VK_NULL_HANDLE;
            if (vkCreateSampler(vulkanResource.GetDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
                vkDestroyImageView(vulkanResource.GetDevice(), imageView, nullptr);
                return std::nullopt;
            }

            stbi_image_free(data);

            auto textureResource = Resources::VkTextureResource(vulkanResource.GetDevice(), std::move(imageResource), imageView, sampler);

            return textureResource;
        }
    } // namespace

    MeshLoader::result_type MeshLoader::operator()(
        Resources::VulkanResource&          vulkanResource,
        VkCommandBuffer                     commandBuffer,
        Resources::VkStagingBufferResource& stagingBuffer,
        const std::string&                  path) const
    {
        Assimp::Importer importer;

        auto loadedModelDescriptorOpt = loadModel(importer, path);
        if (!loadedModelDescriptorOpt) {
            return std::nullopt;
        }

        auto& loadedModelDescriptor = *loadedModelDescriptorOpt;
        if (loadedModelDescriptor.vertices.empty() || loadedModelDescriptor.indices.empty()) {
            std::cerr << "Loaded mesh has no vertices or indices" << std::endl;
            return std::nullopt;
        }

        auto raytracingFlags = [&]() {
            if (vulkanResource.GetAdditionalExtensions() & Resources::VulkanDeviceAdditionalExtensions::RAYTRACING_AVAILABLE) {
                return VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
            }
            return 0;
        }();

        Resources::VkBufferResource<Vertex> vertexBuffer(
            vulkanResource.GetVmaAllocator(),
            loadedModelDescriptor.vertices.size() * sizeof(Vertex),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | raytracingFlags);
        Resources::VkBufferResource<Index> indexBuffer(
            vulkanResource.GetVmaAllocator(),
            loadedModelDescriptor.indices.size() * sizeof(Index),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | raytracingFlags);

        stagingBuffer.Copy(vertexBuffer.GetBuffer(), loadedModelDescriptor.vertices.data(), vertexBuffer.GetBufferSize());
        stagingBuffer.Copy(indexBuffer.GetBuffer(), loadedModelDescriptor.indices.data(), indexBuffer.GetBufferSize());

        // Texture path is relative, so we need to rebuild it.
        if (loadedModelDescriptor.texturePath) {
            std::filesystem::path absolutePath = std::filesystem::path(path).parent_path() / std::filesystem::path(*loadedModelDescriptor.texturePath);
            loadedModelDescriptor.texturePath  = absolutePath.string();
        }
        auto textureOpt = loadTexture(vulkanResource, commandBuffer, stagingBuffer, loadedModelDescriptor.texturePath);

        std::filesystem::path filePath(path);
        auto                  fileName = filePath.stem().string();

        Resources::MeshResource meshResource{
            fileName,
            std::move(vertexBuffer),
            std::move(indexBuffer),
            std::move(loadedModelDescriptor.vertices),
            std::move(loadedModelDescriptor.indices),
            std::move(textureOpt)};

        return {std::make_unique<Resources::MeshResource>(std::move(meshResource))};
    }

} // namespace Prism::Loaders