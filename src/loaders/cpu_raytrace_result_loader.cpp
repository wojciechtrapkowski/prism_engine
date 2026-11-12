#include "loaders/cpu_raytrace_result_loader.hpp"
#include "components/transform.hpp"

#include <stack>

#include "glm/glm.hpp"

namespace Prism::Loaders
{
    namespace
    {
        // TODO: Take from settings.
        constexpr float T_MIN   = 0.0f;
        constexpr float T_MAX   = std::numeric_limits<float>::max();
        constexpr auto  EPSILON = 0.0001f;

        struct Ray
        {
            glm::vec4 origin;
            glm::vec4 direction;
        };

        bool checkRayWithTriangleIntersection(Ray& ray, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
        {
            // Based on:
            // https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle//moller-trumbore-ray-triangle-intersection.html.

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
        }

        bool checkRayWithAABBCollision(Ray& ray, glm::vec4 lower, glm::vec4 upper)
        {
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
                    if (ray.origin.x < lower.x || ray.origin.x > upper.x) {
                        return false;
                    }
                    tXMin = -std::numeric_limits<float>::infinity();
                    tXMax = std::numeric_limits<float>::infinity();
                } else if (ray.direction.x > 0) {
                    tXMin = (lower.x - ray.origin.x) / ray.direction.x;
                    tXMax = (upper.x - ray.origin.x) / ray.direction.x;
                } else {
                    tXMin = (upper.x - ray.origin.x) / ray.direction.x;
                    tXMax = (lower.x - ray.origin.x) / ray.direction.x;
                }
            }
            float tYMin = 0.0f;
            float tYMax = 0.0f;
            {
                if (ray.direction.y == 0) {
                    if (ray.origin.y < lower.y || ray.origin.y > upper.y) {
                        return false;
                    }
                    tYMin = -std::numeric_limits<float>::infinity();
                    tYMax = std::numeric_limits<float>::infinity();
                } else if (ray.direction.y > 0) {
                    tYMin = (lower.y - ray.origin.y) / ray.direction.y;
                    tYMax = (upper.y - ray.origin.y) / ray.direction.y;
                } else {
                    tYMin = (upper.y - ray.origin.y) / ray.direction.y;
                    tYMax = (lower.y - ray.origin.y) / ray.direction.y;
                }
            }
            float tZMin = 0.0f;
            float tZMax = 0.0f;
            {
                if (ray.direction.z == 0) {
                    if (ray.origin.z < lower.z || ray.origin.z > upper.z) {
                        return false;
                    }
                    tZMin = -std::numeric_limits<float>::infinity();
                    tZMax = std::numeric_limits<float>::infinity();
                } else if (ray.direction.z > 0) {
                    tZMin = (lower.z - ray.origin.z) / ray.direction.z;
                    tZMax = (upper.z - ray.origin.z) / ray.direction.z;
                } else {
                    tZMin = (upper.z - ray.origin.z) / ray.direction.z;
                    tZMax = (lower.z - ray.origin.z) / ray.direction.z;
                }
            }

            float tMin = std::max({tXMin, tYMin, tZMin, T_MIN});
            float tMax = std::min({tXMax, tYMax, tZMax, T_MAX});

            return tMin <= tMax;
        }

        size_t traverseBVH(Resources::CPUBVHResource& bvh, Ray& ray)
        {
            auto& nodes = bvh.GetNodes();

            if (nodes.empty()) {
                return 0;
            }

            size_t currentHits = 0;

            // for (auto& node : bvh.nodes) {
            //     auto& aabb = node.aabb;
            //     bool hit = checkRayWithAABBCollision(ray, aabb);
            //     if (hit) {
            //         currentHits++;
            //     }
            // }
            // return currentHits;

            std::stack<uint32_t> indicesStack;
            indicesStack.push(0);

            while (!indicesStack.empty()) {
                // Copy the index before popping to safely inspect it
                auto currentNodeIndex = indicesStack.top();
                indicesStack.pop();

                auto& currentNode = nodes[currentNodeIndex];
                auto& aabb        = currentNode.aabb;

                bool hit = checkRayWithAABBCollision(ray, aabb.lower, aabb.upper);
                if (hit) {
                    // std::cout << std::format("[DFS] AABB Collision: {}\n", "HIT");
                }

                if (hit) {
                    if (currentNode.leaf) {
                        if (checkRayWithTriangleIntersection(ray, currentNode.leaf->v0, currentNode.leaf->v1, currentNode.leaf->v2)) {
                            currentHits++;
                        }
                        // std::cout << std::format("[DFS] Found leaf! Total hits: {}\n", currentHits);
                    } else {
                        // std::cout << "[DFS] Internal node, pushing children to stack:\n";
                        for (size_t i = 0; i < currentNode.numChildren; i++) {
                            auto offset = currentNode.children[i];

                            indicesStack.push(currentNodeIndex + offset);
                        }
                    }
                }
            }

            if (currentHits > 0) {
                // std::cout << std::format("[DFS] Finished. Total hits: {}\n", currentHits);
            }

            return currentHits;
        }
    } // namespace

    // In the future, with TLAS accept just one BVH.
    CPURaytraceResultLoader::result_type CPURaytraceResultLoader::operator()(
        CPURaytraceResultLoader::CPURaytraceSettings&                    settings,
        std::vector<std::reference_wrapper<Resources::CPUBVHResource>>& bvhsToTraverse,
        std::vector<glm::mat4>&                                         transforms) const
    {
        if (transforms.size() != bvhsToTraverse.size()) {
            return std::nullopt;
        }

        // Row major.
        std::vector<Resources::CPURaytraceResultResource::PerPixelData> output;
        output.resize(settings.width * settings.height);

        glm::mat4 invProj = glm::inverse(settings.cameraRelated.projection);
        glm::mat4 invView = glm::inverse(settings.cameraRelated.view);
        for (size_t row = 0; row < settings.height; row++) {
            for (size_t col = 0; col < settings.width; col++) {
                // Sample center of pixels.
                glm::vec4 position = glm::vec4(static_cast<float>(col) + 0.5f, static_cast<float>(row + 0.5f), 0.5f, 1.0f);

                // Convert screen space to NDC.
                position.x = (position.x / settings.width) * 2.0 - 1.0;
                position.y = 1.0f - (position.y / settings.height) * 2.0; // Vulkan Y negation.

                // Convert to clip & world space.
                position = invView * invProj * position;

                glm::vec3 worldPoint = glm::vec3(position) / position.w;
                glm::vec3 cameraPos  = settings.cameraRelated.position;

                Ray ray{};
                ray.origin    = glm::vec4(cameraPos, 1.0f);                              // point, which can be translated so w=1.0f
                ray.direction = glm::vec4(glm::normalize(worldPoint - cameraPos), 0.0f); // direction, w=0.0f;

                // DEBUG
                // uint32_t color = row * RT_HEIGHT + col;

                Resources::CPURaytraceResultResource::PerPixelData pixelData{};
                for (int i = 0; i < bvhsToTraverse.size(); i++) {
                    // We need to transform ray. By inverse of the transform of the mesh, so that ray is in its local space.
                    auto& transform = transforms[i];

                    ray.origin    = glm::inverse(transform) * ray.origin;
                    ray.direction = glm::normalize(glm::inverse(transform) * ray.direction);

                    auto hitsCount = traverseBVH(bvhsToTraverse[i], ray);

                    pixelData.r = hitsCount > 0 ? 255 : 0;
                    pixelData.g = ((hitsCount > 0 ? (255 / bvhsToTraverse.size() * (i + 1)) : 0));
                    pixelData.b = 0;
                    pixelData.a = 255;
                }

                output[row * settings.width + col] = pixelData;
            }
        }

        Resources::CPURaytraceResultResource result(std::move(output), settings.width, settings.height);
        return result;
    };
} // namespace Prism::Loaders