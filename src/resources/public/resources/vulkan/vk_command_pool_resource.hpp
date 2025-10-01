#pragma once

#include "resources/resource.hpp"
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace Prism::Resources {
    struct VkCommandPoolResource : ResourceImpl<VkCommandPoolResource> {
        explicit VkCommandPoolResource(VkDevice device, uint32_t queueFamilyIndex);
        ~VkCommandPoolResource();

        VkCommandPoolResource(const VkCommandPoolResource &) = delete;
        VkCommandPoolResource &operator=(const VkCommandPoolResource &) = delete;

        VkCommandPoolResource(VkCommandPoolResource &&other) noexcept;
        VkCommandPoolResource &operator=(VkCommandPoolResource &&other) noexcept;

        struct Scope {
          public:
            Scope(VkCommandPoolResource &parent, size_t startIndex);
            ~Scope() = default;

            VkCommandBuffer GetNextCommandBuffer();
            void Reset();

            // Container-like API
            size_t size() const noexcept;

            VkCommandBuffer *data() noexcept;
            const VkCommandBuffer *data() const noexcept;

            VkCommandBuffer &operator[](size_t idx);
            const VkCommandBuffer &operator[](size_t idx) const;

          private:
            VkCommandPoolResource &parent;
            size_t startIndex;
            size_t localIndex;
        };

        Scope BeginScope();

        void Reset();


      private:
        friend void swap(VkCommandPoolResource &lhs, VkCommandPoolResource &rhs) noexcept;

        VkCommandBuffer GetNextCommandBuffer();

        static constexpr uint32_t INITIAL_BUFFER_COUNT = 2;

        VkDevice device = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> commandBuffers = {};
        size_t currentBufferIndex = 0;
    };

} // namespace Prism::Resources