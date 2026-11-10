#include "systems/subsystems/mesh_drawing_system/custom_raytraced_geometry_drawing_subsystem.hpp"

#include "components/aabb.hpp"
#include "components/mesh.hpp"
#include "components/camera.hpp"
#include "components/transform.hpp"
#include "components/tags.hpp"
#include "components/systems_settings.hpp"

#include "resources/resource_storage.hpp"
#include "resources/mesh_resource.hpp"

#include <optional>
#include <stack>
#include <format>
#include <string>

#include "glm/glm.hpp"
#include "imgui_impl_vulkan.h"

namespace Prism::Systems::Subsystems::MeshDrawingSystem
{
    namespace
    {
        constexpr auto EPSILON          = 0.0001f;
        constexpr auto MAX_BVH_CHILDREN = 6;

        bool operator>=(glm::vec4& a, glm::vec4 b)
        {
            bool result = true;
            result &= a.x >= b.x;
            result &= a.y >= b.y;
            result &= a.z >= b.z;
            return result;
        }

        bool operator<=(glm::vec4& a, glm::vec4 b)
        {
            bool result = true;
            result &= a.x <= b.x;
            result &= a.y <= b.y;
            result &= a.z <= b.z;
            return result;
        }

        struct Ray
        {
            glm::vec4 origin;
            glm::vec4 direction;
        };

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

            Components::AABB       aabb;
            std::vector<uint32_t>  children; // always positive offsets, from this node to children.
            uint32_t               numChildren;
            std::optional<BVHLeaf> leaf;
        };

        struct BVH : Resources::ResourceImpl<BVH>
        {
            std::vector<BVHNode> nodes;
        };

        // First implementation is on CPU.
        std::vector<BVHNode> buildBVHUsingMorton(Resources::ResourceStorage& meshStorage, entt::registry& registry, entt::entity meshEntity)
        {
            std::vector<BVHNode> bvhNodes{};

            std::vector<std::pair<uint32_t, BVHNode>> bvhLeaves{};

            // TODO: Handle case, where we have zero extent on some axis.
            auto convertTriangleToMortonCode = [](glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, Components::AABB& sceneAABB) {
                glm::vec3 centroid = (v0 + v1 + v2) / 3.0f;

                constexpr float binCount = 1024; // we have 10 bits.

                // First normalize centroid according to scene bounding box. After that, it is in scene space. Range: <0, 1>
                const glm::vec3 normalized = glm::clamp((centroid - glm::vec3(sceneAABB.lower)) / glm::vec3(sceneAABB.upper - sceneAABB.lower), 0.0f, 1.0f);

                // Quantize scene space
                const glm::uvec3 quantized = glm::min(normalized * binCount, binCount - 1.0f);

                // intertwine bits like that:
                // zyx

                uint32_t code = 0;
                
                constexpr auto bitsPerAxis = 10;
                for (int i = 0; i < bitsPerAxis; i++) {
                    code |= (((quantized.x) >> i) & 0x1u) << (3 * i);
                    code |= (((quantized.y) >> i) & 0x1u) << (3 * i + 1);
                    code |= (((quantized.z) >> i) & 0x1u) << (3 * i + 2);
                }

                return code;
            };

            if (!registry.all_of<Components::Mesh>(meshEntity)) {
                throw std::runtime_error("Mesh resource not available!");
            }
            auto meshComponent = registry.get<Components::Mesh>(meshEntity);

            auto meshOpt = meshStorage.Get<Resources::MeshResource>(meshComponent.resourceId);
            if (!meshOpt) {
                throw std::runtime_error("Mesh not available!");
            }
            auto& mesh = meshOpt->get();
            
            // At this point, mesh will contain AABB calculated from physics system, if not, we should calculate it.
            if (!registry.all_of<Components::AABB>(meshEntity)) {
                auto numericLimitsMin = std::numeric_limits<float>::lowest();
                auto numericLimitsMax = std::numeric_limits<float>::max();

                glm::vec4 aabbLower{numericLimitsMax};
                glm::vec4 aabbUpper{numericLimitsMin};

                for (const auto& vertex : mesh.GetVertices()) {
                    aabbLower.x = std::min(aabbLower.x, vertex.position.x);
                    aabbLower.y = std::min(aabbLower.y, vertex.position.y);
                    aabbLower.z = std::min(aabbLower.z, vertex.position.z);
                    aabbLower.w = 1.0f;

                    aabbUpper.x = std::max(aabbUpper.x, vertex.position.x);
                    aabbUpper.y = std::max(aabbUpper.y, vertex.position.y);
                    aabbUpper.z = std::max(aabbUpper.z, vertex.position.z);
                    aabbUpper.w = 1.0f;
                }

                registry.emplace<Components::AABB>(meshEntity, aabbLower, aabbUpper, aabbLower, aabbUpper);
            }
            Components::AABB& sceneAABB = registry.get<Components::AABB>(meshEntity);

            auto& meshIndices  = mesh.GetIndices();
            auto& meshVertices = mesh.GetVertices();
            for (size_t i = 0; i < meshIndices.size(); i += 3) {
                auto idx0 = meshIndices[i].idx;
                auto idx1 = meshIndices[i + 1].idx;
                auto idx2 = meshIndices[i + 2].idx;

                glm::vec3 v0 = meshVertices[idx0].position;
                glm::vec3 v1 = meshVertices[idx1].position;
                glm::vec3 v2 = meshVertices[idx2].position;

                uint32_t mortonCode = convertTriangleToMortonCode(v0, v1, v2, sceneAABB);

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
            

            std::sort(bvhLeaves.begin(), bvhLeaves.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

            // Now we can create our BVH - leaves are sorted by Morton codes.
            bvhNodes.reserve(bvhLeaves.size()); // This could be improved.

            std::vector<size_t> currentLevel;
            currentLevel.reserve(bvhNodes.size());
            for (const auto& [mortonCode, node] : bvhLeaves) {
                currentLevel.push_back(bvhNodes.size());
                bvhNodes.push_back(node);
            }

            while (currentLevel.size() > 1) {
                std::vector<size_t> nextLevel;

                while (!currentLevel.empty()) {
                    auto numChildren = std::min(currentLevel.size(), static_cast<size_t>(MAX_BVH_CHILDREN));
                    
                    auto parentIndex = bvhNodes.size();

                    BVHNode parentNode{};
                    parentNode.aabb.lower  = glm::vec4(std::numeric_limits<float>::max());
                    parentNode.aabb.upper = glm::vec4(std::numeric_limits<float>::lowest());
                     
                    parentNode.numChildren = numChildren;
                    parentNode.children.resize(numChildren);

                    for (size_t i = 0; i < numChildren; i++) {
                        auto childIndex = currentLevel.back();
                        currentLevel.pop_back();

                        auto& child = bvhNodes[childIndex];
                        parentNode.aabb.lower = glm::min(parentNode.aabb.lower, child.aabb.lower);
                        parentNode.aabb.upper = glm::max(parentNode.aabb.upper, child.aabb.upper);

                        parentNode.children[i] = parentIndex - childIndex;
                    }
                    bvhNodes.push_back(std::move(parentNode));
                    nextLevel.push_back(parentIndex);
                }

                currentLevel = std::move(nextLevel);
            }

            std::reverse(bvhNodes.begin(), bvhNodes.end());

            return bvhNodes;
        }

        std::vector<uint32_t> raytraceScene(entt::registry& registry, std::vector<std::reference_wrapper<BVH>> bvhsToTraverse, std::vector<entt::entity> meshEntities, entt::entity cameraEntity, int width, int height)
        {
            // TODO: Take from settings.
            constexpr float T_MIN = 0.0f;
            constexpr float T_MAX = std::numeric_limits<float>::max();

            // We don't perform a check here, it was checked at the caller's side.
            Components::Camera& camera = registry.get<Components::Camera>(cameraEntity);
            Components::Transform& cameraTransform = registry.get<Components::Transform>(cameraEntity);

            // Row major.
            std::vector<uint32_t> output;
            output.resize(width * height);

            const auto checkRayWithTriangleCollision = [T_MIN, T_MAX](Ray& ray, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) { 
                // Based on: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle//moller-trumbore-ray-triangle-intersection.html.

                glm::vec3 v0v1 = v1 - v0;
                glm::vec3 v0v2 = v2 - v0;
                glm::vec3 t    = glm::vec3(ray.origin) - v0;

                glm::vec3 p = glm::cross(glm::vec3(ray.direction), v0v2);
                glm::vec3 q = glm::cross(t, v0v1);

                float det = (glm::dot(p, v0v1));

                // Back face culling + parallel ray handling (no intersection case).
                if (det < EPSILON) {
                    return false;
                }

                float invDet = 1 / det;

                float tResult = invDet * glm::dot(q, v0v2);
                float uResult = invDet * glm::dot(p, t);
                float vResult = invDet * glm::dot(q, glm::vec3(ray.direction));

                if (tResult < 0.0f) {
                    return false;    
                }

                if (tResult > T_MAX) {
                    return false;
                }

                if (uResult < 0.0f || uResult > 1.0f) {
                    return false;
                }

                if (vResult < 0.0f || uResult + vResult > 1.0f) {
                    return false;
                }

                return true;
            };

            const auto checkRayWithAABBCollision = [T_MIN, T_MAX](Ray& ray, Components::AABB& aabb) {
                // smit's slab method. TODO: improve it based on the paper.
                
                // r(t) = dt + o;
                // t_min = lower
                // t_max = upper
                // consider case where ray go in different side
                // (upper.x - ray.o.x) / ray .d.x - t_max

                // check if calculated t values overlap

                // leaves also have bounding boxes. we should detect tham and check if they overlap using another method.
                // for now assume, if ray hit their aabb, they are hit.

                float tXMin = 0.0f;
                float tXMax = 0.0f;
                {
                    if (ray.direction.x == 0) {
                        if (ray.origin.x < aabb.lower.x || ray.origin.x > aabb.upper.x) {
                            return false;
                        }
                        tXMin = -std::numeric_limits<float>::infinity();
                        tXMax = std::numeric_limits<float>::infinity();
                    }
                    else if (ray.direction.x > 0) {
                        tXMin = (aabb.lower.x - ray.origin.x) / ray.direction.x;
                        tXMax = (aabb.upper.x - ray.origin.x) / ray.direction.x;
                    } else {
                        tXMin = (aabb.upper.x - ray.origin.x) / ray.direction.x;
                        tXMax = (aabb.lower.x - ray.origin.x) / ray.direction.x;
                    }
                }
                float tYMin = 0.0f;
                float tYMax = 0.0f;
                {
                    if (ray.direction.y == 0) {
                        if (ray.origin.y < aabb.lower.y || ray.origin.y > aabb.upper.y) {
                            return false;
                        }
                        tYMin = -std::numeric_limits<float>::infinity();
                        tYMax = std::numeric_limits<float>::infinity();
                    }
                    else if (ray.direction.y > 0) {
                        tYMin = (aabb.lower.y - ray.origin.y) / ray.direction.y;
                        tYMax = (aabb.upper.y - ray.origin.y) / ray.direction.y;
                    } else {
                        tYMin = (aabb.upper.y - ray.origin.y) / ray.direction.y;
                        tYMax = (aabb.lower.y - ray.origin.y) / ray.direction.y;
                    }
                }
                float tZMin = 0.0f;
                float tZMax = 0.0f;
                {
                    if (ray.direction.z == 0) {
                        if (ray.origin.z < aabb.lower.z || ray.origin.z > aabb.upper.z) {
                            return false;
                        }
                        tZMin = -std::numeric_limits<float>::infinity();
                        tZMax = std::numeric_limits<float>::infinity();
                    }
                    else if (ray.direction.z > 0) {
                        tZMin = (aabb.lower.z - ray.origin.z) / ray.direction.z;
                        tZMax = (aabb.upper.z - ray.origin.z) / ray.direction.z;
                    } else {
                        tZMin = (aabb.upper.z - ray.origin.z) / ray.direction.z;
                        tZMax = (aabb.lower.z - ray.origin.z) / ray.direction.z;
                    }
                }

                float tMin = std::max({tXMin, tYMin, tZMin, T_MIN});
                float tMax = std::min({tXMax, tYMax, tZMax, T_MAX});
                
                return tMin <= tMax;
            };

            const auto& dfs = [checkRayWithAABBCollision, checkRayWithTriangleCollision](BVH& bvh, Ray& ray) -> size_t {
                if (bvh.nodes.empty()) {
                    return 0;
                }

                size_t currentHits = 0;

                //for (auto& node : bvh.nodes) {
                //    auto& aabb = node.aabb;
                //    bool hit = checkRayWithAABBCollision(ray, aabb);
                //    if (hit) {
                //        currentHits++;
                //    }
                //}
                //return currentHits;

                std::stack<uint32_t> indicesStack;
                indicesStack.push(0);

                while (!indicesStack.empty()) {
                    // Copy the index before popping to safely inspect it
                    auto currentNodeIndex = indicesStack.top();
                    indicesStack.pop();

                    auto& currentNode = bvh.nodes[currentNodeIndex];
                    auto& aabb        = currentNode.aabb;

                    bool hit = checkRayWithAABBCollision(ray, aabb);
                    if (hit) {
                        //std::cout << std::format("[DFS] AABB Collision: {}\n", "HIT");
                    }

                    if (hit) {
                        if (currentNode.leaf) {
                            if (checkRayWithTriangleCollision(ray, currentNode.leaf->v0, currentNode.leaf->v1, currentNode.leaf->v2)) {
                                currentHits++; 
                            }
                            //std::cout << std::format("[DFS] Found leaf! Total hits: {}\n", currentHits);
                        } else {
                            //std::cout << "[DFS] Internal node, pushing children to stack:\n";
                            for (size_t i=0; i<currentNode.numChildren; i++) {
                                auto offset = currentNode.children[i];

                                indicesStack.push(currentNodeIndex + offset);
                            }
                        }
                    }
                }

                if (currentHits > 0) {
                     //std::cout << std::format("[DFS] Finished. Total hits: {}\n", currentHits); 
                }

                return currentHits;
            };

            glm::mat4 invProj = glm::inverse(camera.projection);
            glm::mat4 invView = glm::inverse(camera.view);
            for (size_t row = 0; row < height; row++) {
                for (size_t col = 0; col < width; col++) {
                    // Sample center of pixels.
                    glm::vec4 position = glm::vec4(static_cast<float>(col) + 0.5f, static_cast<float>(row + 0.5f), 0.5f, 1.0f);

                    // Convert screen space to NDC.
                    position.x = (position.x / width) * 2.0 - 1.0;
                    position.y = 1.0f - (position.y / height) * 2.0; // Vulkan Y negation.

                    // Convert to clip & world space.
                    position = invView * invProj * position;

                    glm::vec3 worldPoint = glm::vec3(position) / position.w;
                    glm::vec3 cameraPos = cameraTransform.transform[3];

                    Ray ray{};
                    ray.origin    = glm::vec4(cameraPos, 1.0f); // point, which can be translated so w=1.0f
                    ray.direction = glm::vec4(glm::normalize(worldPoint - cameraPos), 0.0f); // direction, w=0.0f;

                    // DEBUG
                    // uint32_t color = row * RT_HEIGHT + col;

                    uint32_t color = 0;
                    for (int i = 0; i < bvhsToTraverse.size(); i++) {
                        // We need to transform ray. By inverse of the transform of the mesh, so that ray is in its local space.
                        auto currentMeshEntity = meshEntities[i];
                        if (registry.all_of<Components::Transform>(currentMeshEntity)) {
                            auto& transform = registry.get<Components::Transform>(currentMeshEntity);
                            
                            ray.origin    = glm::inverse(transform.transform) * ray.origin;
                            ray.direction = glm::normalize(glm::inverse(transform.transform) * ray.direction);
                        }


                        auto hitsCount = dfs(bvhsToTraverse[i], ray);

                        color |= hitsCount > 0 ? 255 : 0;         // r
                        color |= ((hitsCount > 0 ? ( 255 / bvhsToTraverse.size() * (i+1)) : 0)  << 8);                        // g
                        color |= (0 << 16);                       // b
                        color |= (0xFF << 24);                    // a
                        
                    }

                    output[row * width + col] = color;
                }
            }

            return output;
        }
    } // namespace

    CustomRaytracedGeometryDrawingSubsystem::CustomRaytracedGeometryDrawingSubsystem(Resources::ContextResources& contextResources) :
        _contextResources(contextResources)
    {
        _onCpuRaytraceStartEventConnection =
            _contextResources.GetDispatcher().sink<Events::CPURaytraceStartEvent>().connect<&CustomRaytracedGeometryDrawingSubsystem::onCPURaytraceStartEvent>(
                this);
    };

    CustomRaytracedGeometryDrawingSubsystem::~CustomRaytracedGeometryDrawingSubsystem() {}

    void CustomRaytracedGeometryDrawingSubsystem::Update(
        float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::VkStagingBufferResource& stagingBuffer)
    {
        constexpr auto EACH_AABB_IS_ENTITY = true;

        auto& vulkanResource = _contextResources.GetVulkanResource();
        auto& registry       = scene.GetRegistry();
        auto& systemsStorage = scene.GetSystemsStorage();
        auto& meshStorage    = scene.GetMeshStorage();

        Components::MeshDrawingSystemSettings meshDrawingSystemSettings{};
        auto meshDrawingSystemSettingsView = registry.view<Components::MeshDrawingSystemSettings>();
        if (!meshDrawingSystemSettingsView.empty()) {
            meshDrawingSystemSettings = registry.get<Components::MeshDrawingSystemSettings>(meshDrawingSystemSettingsView.front());
        }

        auto getMeshBVHBufferId = [](auto& resourceId) {
            return std::hash<std::string_view>{}(std::format("{}/{}", MESH_BVH_BUFFER_ID_PREFIX, std::to_string(resourceId)));
        };

        auto meshView = registry.view<Components::Mesh>();
        for (auto&& [entity, meshComponent] : meshView.each()) {
            auto id = getMeshBVHBufferId(meshComponent.resourceId);

            if (!systemsStorage.Contains(id)) {
                if (!meshStorage.Contains(meshComponent.resourceId)) {
                    continue;
                }
                auto bvhNodes = buildBVHUsingMorton(meshStorage, registry, entity);
                
                if (EACH_AABB_IS_ENTITY) {
                    for (auto& node : bvhNodes) {
                        auto entity             = registry.create();
                        node.aabb.originalLower = node.aabb.lower;
                        node.aabb.originalUpper = node.aabb.upper;
                        registry.emplace<Components::AABB>(entity, node.aabb);
                    }
                }

                BVH  bvh{};
                bvh.nodes = std::move(bvhNodes);
                systemsStorage.Insert(id, std::make_unique<BVH>(std::move(bvh)));
            }
        }

        auto activeCameraView = registry.view<Components::Tags::ActiveCamera>();

        std::vector<std::reference_wrapper<BVH>> bvhsToTraverse;
        std::vector<entt::entity>                meshEntities;
        for (auto&& [entity, meshComponent] : meshView.each()) {
            auto id     = getMeshBVHBufferId(meshComponent.resourceId);
            auto bvhOpt = systemsStorage.Get<BVH>(id);
            if (bvhOpt) {
                bvhsToTraverse.push_back(bvhOpt.value());
                meshEntities.push_back(entity);
            }
        }

        bool runCPURaytrace = true;
        runCPURaytrace &= _cpuRaytraceStartEventToProcessOpt.has_value();
        runCPURaytrace &= !activeCameraView.empty();
        runCPURaytrace &= !bvhsToTraverse.empty();

        auto cameraEntity = activeCameraView.front();

        runCPURaytrace &= registry.all_of<Components::Camera, Components::Transform>(cameraEntity);

        if (runCPURaytrace) {
            auto height         = meshDrawingSystemSettings.customRaytracedGeometryDrawingSystemSettings.raytracingHeight;
            auto width          = meshDrawingSystemSettings.customRaytracedGeometryDrawingSystemSettings.raytracingWidth;

            auto raytraceResult = raytraceScene(
                registry,
                bvhsToTraverse,
                meshEntities,
                cameraEntity,
                width,
                height);

            const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * 4); // 4 bytes.

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
                throw std::runtime_error("Error: couldn't create image from raytracing result.");
            }

            Resources::VkImageResource imageResource(vulkanResource.GetVmaAllocator(), textureAllocation, textureImage, width, height);

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

            stagingBuffer.CopyToImage(imageResource.GetImage(), raytraceResult.data(), imageSize, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image            = imageResource.GetImage();
            viewInfo.viewType         = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format           = VK_FORMAT_R8G8B8A8_SRGB;
            viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            VkImageView imageView = VK_NULL_HANDLE;
            if (vkCreateImageView(vulkanResource.GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                throw std::runtime_error("Error: couldn't create image view from raytracing result.");
            }

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
                throw std::runtime_error("Error: couldn't create image sampler from raytracing result.");
            }

            auto textureResource = Resources::VkTextureResource(vulkanResource.GetDevice(), std::move(imageResource), imageView, sampler);

            Resources::Resource::ID textureId =
                std::hash<std::string_view>{}(std::format("{}_{}", CPU_RAYTRACE_RESULTS_BUFFER_ID_PREFIX, _cpuRaytraceCounter++));

            systemsStorage.Insert(textureId, std::make_unique<Resources::VkTextureResource>(std::move(textureResource)));
        }
    }

    void CustomRaytracedGeometryDrawingSubsystem::Render(
        float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget)
    {
        auto& systemsStorage = scene.GetSystemsStorage();

        Resources::Resource::ID textureId = std::hash<std::string_view>{}(std::format("{}_{}", CPU_RAYTRACE_RESULTS_BUFFER_ID_PREFIX, _cpuRaytraceCounter - 1));
        auto                    textureResourceOpt = systemsStorage.Get<Resources::VkTextureResource>(textureId);
        if (_cpuRaytraceStartEventToProcessOpt && textureResourceOpt) {
            auto& textureResource = textureResourceOpt->get();

            auto imguiTextureId =
                ImGui_ImplVulkan_AddTexture(textureResource.GetSampler(), textureResource.GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            _contextResources.GetDispatcher().enqueue<Events::CPURaytraceFinishedEvent>(imguiTextureId, textureResource.GetWidth(), textureResource.GetHeight());
        }
        _cpuRaytraceStartEventToProcessOpt = std::nullopt;
    }

    void CustomRaytracedGeometryDrawingSubsystem::onCPURaytraceStartEvent(Events::CPURaytraceStartEvent& event)
    {
        _cpuRaytraceStartEventToProcessOpt = event;
    }
} // namespace Prism::Systems::Subsystems::MeshDrawingSystem