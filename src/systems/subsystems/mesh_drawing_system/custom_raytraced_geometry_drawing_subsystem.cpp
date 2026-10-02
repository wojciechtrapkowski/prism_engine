#include "systems/subsystems/mesh_drawing_system/custom_raytraced_geometry_drawing_subsystem.hpp"

#include "components/aabb.hpp"
#include "components/mesh.hpp"

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

            constexpr static auto MAX_NUMBER_OF_CHILDREN = 6;

            Components::AABB                            aabb;
            std::array<BVHNode, MAX_NUMBER_OF_CHILDREN> children;
            std::optional<BVHLeaf>                      leaf;
        };

        struct BVH
        {
            BVHNode root;
        };

        // First implementation is on CPU.
        void buildBVHUsingMorton(Resources::ResourceStorage& meshStorage, entt::registry& registry)
        {
            BVH outputBVH{};

            // We are doing bottom up, so we can't start with the root.
            std::vector<std::vector<BVHNode>> bvhNodes{};
            bvhNodes.resize(1);

            std::vector<std::pair<uint32_t, BVHNode>> bvhLeaves{};

            auto meshView = registry.view<Components::Mesh>();

            auto convertTriangleToMortonCode = [](glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
                // calculate centroid
                // quantize
                // convert to code

                return 0;
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
                    aabb.originalLower = glm::vec4(glm::min(v0, glm::min(v1, v2)), 1.0);
                    aabb.originalUpper = glm::vec4(glm::max(v0, glm::max(v1, v2)), 1.0);

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

            int currentIndex = 1;
            while (true) { // current index < log_6
                int                  currentCounter = 0;
                std::vector<BVHNode> tempNodes;
                Components::AABB     tempAabb;
                for (size_t i = 0; i < bvhNodes[currentIndex - 1].size(); i++) {
                    currentCounter++;

                    if (currentCounter % 6 == 0) {
                        // calculate aabb
                        // create new node
                    }
                }
            }
        }

    } // namespace

    CustomRaytracedGeometryDrawingSubsystem::CustomRaytracedGeometryDrawingSubsystem(Resources::ContextResources& contextResources) :
        _contextResources(contextResources) {};

    CustomRaytracedGeometryDrawingSubsystem::~CustomRaytracedGeometryDrawingSubsystem() {}

    void CustomRaytracedGeometryDrawingSubsystem::Update(
        float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::VkStagingBufferResource& stagingBuffer)
    {}

    void CustomRaytracedGeometryDrawingSubsystem::Render(
        float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget)
    {}
} // namespace Prism::Systems::Subsystems::MeshDrawingSystem