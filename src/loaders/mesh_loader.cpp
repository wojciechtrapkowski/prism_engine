#include "loaders/mesh_loader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Prism::Loaders {
    namespace {


        constexpr std::string_view MODELS_DIR = "models/";

        constexpr unsigned int MODELS_LOADING_FLAGS = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices |
                                                      aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;

        using Vertex = Resources::MeshResource::Vertex;
        using Index = Resources::MeshResource::Index;

        struct MeshDescriptor {
            std::vector<Vertex> vertices;
            std::vector<Index> indices;
        };

        glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4 &aiMat) {
            return glm::mat4(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1, aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2, aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3, aiMat.a4,
                             aiMat.b4, aiMat.c4, aiMat.d4);
        }

        void processNode(const aiNode *node, const aiScene *scene, glm::mat4 parentTransform, std::vector<Vertex> &vertices, std::vector<Index> &indices,
                         size_t &vertexOffset) {

            glm::mat4 nodeTransform = aiMatrix4x4ToGlm(node->mTransformation);
            glm::mat4 globalTransform = parentTransform * nodeTransform;

            for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
                aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

                for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                    const aiVector3D &pos = mesh->mVertices[v];

                    glm::vec4 transformedPos = globalTransform * glm::vec4(pos.x, pos.y, pos.z, 1.0f);

                    Vertex vert;
                    vert.position = {transformedPos.x, transformedPos.y, transformedPos.z};
                    vert.normal = {mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z};

                    vertices.push_back(std::move(vert));
                }

                for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
                    const aiFace &face = mesh->mFaces[f];
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

        std::optional<MeshDescriptor> loadModel(Assimp::Importer &importer, const std::string &path) {
            const aiScene *scene = importer.ReadFile(std::string(MODELS_DIR) + path, MODELS_LOADING_FLAGS);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
                return std::nullopt;
            }

            std::vector<Vertex> vertices;
            std::vector<Index> indices;
            size_t vertexOffset = 0;

            processNode(scene->mRootNode, scene, glm::mat4(1.0f), vertices, indices, vertexOffset);

            return MeshDescriptor{.vertices = std::move(vertices), .indices = std::move(indices)};
        }
    } // namespace

    MeshLoader::result_type MeshLoader::operator()(Resources::VulkanResource &vulkanResource, Resources::VkStagingBufferResource &stagingBuffer,
                                                   const std::string &path) const {
        Assimp::Importer importer;

        auto loadedModelDescriptorOpt = loadModel(importer, path);
        if (!loadedModelDescriptorOpt) {
            return std::nullopt;
        }

        auto &loadedModelDescriptor = *loadedModelDescriptorOpt;
        Resources::VkBufferResource<Vertex> vertexBuffer(vulkanResource.GetVmaAllocator(), loadedModelDescriptor.vertices.size() * sizeof(Vertex),
                                                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        Resources::VkBufferResource<Index> indexBuffer(vulkanResource.GetVmaAllocator(), loadedModelDescriptor.indices.size() * sizeof(Index),
                                                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        stagingBuffer.Copy(vertexBuffer.GetBuffer(), loadedModelDescriptor.vertices.data(), vertexBuffer.GetBufferSize());
        stagingBuffer.Copy(indexBuffer.GetBuffer(), loadedModelDescriptor.indices.data(), indexBuffer.GetBufferSize());

        Resources::MeshResource meshResource{std::move(vertexBuffer), std::move(indexBuffer)};

        return {std::make_unique<Resources::MeshResource>(std::move(meshResource))};
    }

} // namespace Prism::Loaders