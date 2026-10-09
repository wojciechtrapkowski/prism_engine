#include "systems/subsystems/mesh_drawing_system/custom_raytraced_geometry_drawing_subsystem.hpp"

#include "components/aabb.hpp"
#include "components/mesh.hpp"
#include "components/camera.hpp"

#include "resources/resource_storage.hpp"
#include "resources/mesh_resource.hpp"

#include <optional>

#include "glm/glm.hpp"

namespace Prism::Systems::Subsystems::MeshDrawingSystem
{
    namespace
    {
        struct BVHNode
        {
            struct BVHLeaf
            {
                entt::entity meshEntity;
                uint32_t     triangleId;

                glm::vec3 v0;
                glm::vec3 v1;
                glm::vec3 v2;
            };

            struct BVHIndex
            {
                uint32_t level;
                uint32_t indexInLevel;
            };

            constexpr static auto MAX_NUMBER_OF_CHILDREN = 6;

            Components::AABB                             aabb;
            std::array<BVHIndex, MAX_NUMBER_OF_CHILDREN> children;
            std::optional<BVHLeaf>                       leaf;
        };

        struct BVH : Resources::ResourceImpl<BVH>
        {
            std::vector<std::vector<BVHNode>> nodes;
        };

        // First implementation is on CPU.
        std::vector<std::vector<BVHNode>> buildBVHUsingMorton(Resources::ResourceStorage& meshStorage, entt::registry& registry)
        {
            // We are doing bottom up, so we can't start with the root.
            std::vector<std::vector<BVHNode>> bvhNodes{};
            bvhNodes.resize(1);

            std::vector<std::pair<uint32_t, BVHNode>> bvhLeaves{};

            auto meshView = registry.view<Components::Mesh>();

            auto convertTriangleToMortonCode = [](glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
                glm::vec3 centroid = (v0 + v1 + v2) / 3.0f;

                int        maxValue = std::pow(2, 9) - 1;
                int        x        = glm::clamp((int)centroid.x, -maxValue, maxValue);
                int        y        = glm::clamp((int)centroid.x, -maxValue, maxValue);
                int        z        = glm::clamp((int)centroid.x, -maxValue, maxValue);
                glm::ivec3 centroidQuantized(x, y, z);

                // intertwine bits like that:
                // int maskX = 0x00100100100100100100100100100100b;
                // int maskY = 0x00010010010010010010010010010010b;
                // int maskZ = 0x00001001001001001001001001001001b;

                int code = 0;
                for (int i = 0; i < 30; i += 3) {
                    code = (code << i) | ((x >> (i / 3)) & 0x1b);
                    code = (code << i + 1) | ((y >> (i / 3)) & 0x1b);
                    code = (code << i + 2) | ((z >> (i / 3)) & 0x1b);
                }

                return code;
            };

            for (auto&& [meshEntity, meshComponent] : meshView.each()) {
                auto meshOpt = meshStorage.Get<Resources::MeshResource>(meshComponent.resourceId);
                if (!meshOpt) {
                    continue;
                }

                auto& mesh = meshOpt->get();

                auto& meshIndices  = mesh.GetIndices();
                auto& meshVertices = mesh.GetVertices();
                for (size_t i = 0; i < meshIndices.size(); i += 3) {
                    auto idx0 = meshIndices[i].idx;
                    auto idx1 = meshIndices[i + 1].idx;
                    auto idx2 = meshIndices[i + 2].idx;

                    glm::vec3 v0 = meshVertices[idx0].position;
                    glm::vec3 v1 = meshVertices[idx1].position;
                    glm::vec3 v2 = meshVertices[idx2].position;

                    uint32_t mortonCode = convertTriangleToMortonCode(v0, v1, v2);

                    auto triangleId = i / 3;

                    BVHNode::BVHLeaf leaf{};
                    leaf.meshEntity = meshEntity;
                    leaf.triangleId = triangleId;
                    leaf.v0         = v0;
                    leaf.v1         = v1;
                    leaf.v2         = v2;

                    Components::AABB aabb;
                    aabb.lower = glm::vec4(glm::min(v0, glm::min(v1, v2)), 1.0);
                    aabb.upper = glm::vec4(glm::max(v0, glm::max(v1, v2)), 1.0);

                    BVHNode node{};
                    node.leaf     = std::move(leaf);
                    node.aabb     = std::move(aabb);
                    node.children = {};

                    bvhLeaves.push_back(std::make_pair(mortonCode, std::move(node)));
                }
            }

            std::sort(bvhLeaves.begin(), bvhLeaves.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

            // Now we can create our BVH - leaves are sorted by Morton codes.
            bvhNodes[0].reserve(bvhLeaves.size());
            for (const auto& [mortonCode, node] : bvhLeaves) {
                bvhNodes[0].push_back(node);
            }

            int      currentIndex = 1;
            uint32_t maxHeight    = std::ceil(std::log(bvhNodes[0].size()) / std::log(6)) + 1;
            while (currentIndex < maxHeight) {
                int                  currentCounter = 0;
                std::vector<BVHNode> tempNodes;
                tempNodes.reserve(std::log(bvhNodes[currentIndex - 1].size()) / std::log(6));
                Components::AABB tempAabb;
                tempAabb.lower = glm::vec4(std::numeric_limits<float>::max());
                tempAabb.upper = glm::vec4(std::numeric_limits<float>::min());

                for (size_t i = 0; i < bvhNodes[currentIndex - 1].size(); i++) {
                    currentCounter++;
                    auto& currentAABB = bvhNodes[currentIndex - 1][i].aabb;

                    // calculate aabb
                    tempAabb.lower = glm::min(tempAabb.lower, currentAABB.lower);
                    tempAabb.upper = glm::max(tempAabb.upper, currentAABB.upper);

                    if (currentCounter % 6 == 0) {
                        // create new node
                        BVHNode node{};
                        node.aabb = tempAabb;
                        for (int j = 0; j < 6; j++) {
                            node.children[j].level        = currentIndex;
                            node.children[j].indexInLevel = i - j;
                        }

                        tempAabb.lower = glm::vec4(std::numeric_limits<float>::max());
                        tempAabb.upper = glm::vec4(std::numeric_limits<float>::min());
                        currentCounter = 0;

                        tempNodes.push_back(node);
                    }
                }

                // create remaining node
                if (currentCounter > 0) {
                    BVHNode node{};
                    node.aabb = tempAabb;
                    for (int j = 0; j < 6; j++) {
                        node.children[j].level        = currentIndex;
                        node.children[j].indexInLevel = bvhNodes[currentIndex - 1].size() - 1 - j;
                    }

                    tempNodes.push_back(node);
                }

                // add new level
                currentIndex++;
                bvhNodes.push_back(std::move(tempNodes));
            }
            return bvhNodes;
        }

        // Maybe VK texture so could you put it into UI?
        void raytraceScene(Resources::RenderTargetResource& renderTarget, Resources::BVH& bvh, Components::Camera& camera)
        {
            constexpr auto RT_WIDTH = 512;
            constexpr auto RT_HEIGHT = 256;

            // cast a ray, so launch a thread to go over the BVH.
            // if it hits a leaf, take its vertices, convert to screen space and colour the pixel.
            // more alpha where more rays hit.

            glm::mat4 invProj = glm::inverse(camera.projection);
            glm::mat4 invView = glm::inverse(camera.view);
            for (size_t row=0; row<RT_HEIGHT; row++) {
                for (size_t col=0; col<RT_WIDTH; col++) {
                    // convert screen space row, col into world space.
                    // so multiply by inverse proj view.
                    // cast a ray from camera to this point in world space.

                    // r(t) = at + b;
                }
            }
        }
    } // namespace

    CustomRaytracedGeometryDrawingSubsystem::CustomRaytracedGeometryDrawingSubsystem(Resources::ContextResources& contextResources) :
        _contextResources(contextResources){};

    CustomRaytracedGeometryDrawingSubsystem::~CustomRaytracedGeometryDrawingSubsystem() {}

    void CustomRaytracedGeometryDrawingSubsystem::Update(
        float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::VkStagingBufferResource& stagingBuffer)
    {
        // Based on the built BVH, create new entities.

        auto& registry       = scene.GetRegistry();
        auto& systemsStorage = scene.GetSystemsStorage();
        auto  bvhOpt         = systemsStorage.Get<BVH>(BVH_BUFFER_ID);
        if (!bvhOpt) {
            auto bvhNodes = buildBVHUsingMorton(scene.GetMeshStorage(), registry);
            for (auto& nodes : bvhNodes) {
                for (auto& node : nodes) {
                    auto entity             = registry.create();
                    node.aabb.originalLower = node.aabb.lower;
                    node.aabb.originalUpper = node.aabb.upper;
                    registry.emplace<Components::AABB>(entity, node.aabb);
                }
            }
            BVH bvh{};
            bvh.nodes = std::move(bvhNodes);

            systemsStorage.Insert(BVH_BUFFER_ID, std::make_unique<BVH>(std::move(bvh)));
        }
    }

    void CustomRaytracedGeometryDrawingSubsystem::Render(
        float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget)
    {
        // Run raytracer
        // Each thread goes into BVH and outputs a colour, only when it hits a triangle.
        // Run compute shader to copy from buffer to render target
    }
} // namespace Prism::Systems::Subsystems::MeshDrawingSystem