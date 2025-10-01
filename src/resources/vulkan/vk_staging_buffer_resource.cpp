#include "resources/vulkan/vk_staging_buffer_resource.hpp"

#include <stdexcept>
#include <utility>

namespace Prism::Resources {
    VkStagingBufferResource::VkStagingBufferResource(VmaAllocator allocator) : allocator(allocator) {
        stagingBuffer =
            VkBufferResource<>(allocator, INITIAL_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        currentlyUtilized = 0;
    }

    VkStagingBufferResource::VkStagingBufferResource(VkStagingBufferResource &&other) noexcept { swap(*this, other); }

    VkStagingBufferResource &VkStagingBufferResource::operator=(VkStagingBufferResource &&other) noexcept {
        if (this != &other) {
            swap(*this, other);
        }
        return *this;
    }

    void VkStagingBufferResource::Copy(VkBuffer destination, void *data, size_t size) {
        if (size + currentlyUtilized > stagingBuffer.GetBufferSize()) {
            size_t newSize = std::max(stagingBuffer.GetBufferSize() * 2, static_cast<VkDeviceSize>(size + currentlyUtilized));

            VkBufferResource<> newBuffer(allocator, newSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            // Copy old contents & replace previous buffer with the new one.
            if (currentlyUtilized > 0) {
                void *oldData = nullptr;
                vmaMapMemory(allocator, stagingBuffer.GetAllocation(), &oldData);

                void *newData = nullptr;
                vmaMapMemory(allocator, newBuffer.GetAllocation(), &newData);

                std::memcpy(newData, oldData, currentlyUtilized);

                vmaUnmapMemory(allocator, newBuffer.GetAllocation());
                vmaUnmapMemory(allocator, stagingBuffer.GetAllocation());
            }

            stagingBuffer = std::move(newBuffer);
        }

        void *mappedData;
        vmaMapMemory(allocator, stagingBuffer.GetAllocation(), &mappedData);
        std::memcpy(static_cast<char *>(mappedData) + currentlyUtilized, data, size);
        vmaUnmapMemory(allocator, stagingBuffer.GetAllocation());

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = currentlyUtilized;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;

        pendingCopies.push_back({destination, copyRegion});

        currentlyUtilized += size;
    }

    void VkStagingBufferResource::Commit(VkCommandBuffer commandBuffer) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        if (pendingCopies.empty()) {
            vkEndCommandBuffer(commandBuffer);
            return;
        }

        for (const auto &pending : pendingCopies) {
            vkCmdCopyBuffer(commandBuffer, stagingBuffer.GetBuffer(), pending.destination, 1, &pending.region);
        }

        pendingCopies.clear();
        currentlyUtilized = 0;

        vkEndCommandBuffer(commandBuffer);
    }

    void swap(VkStagingBufferResource &first, VkStagingBufferResource &second) noexcept {
        using std::swap;
        swap(first.allocator, second.allocator);
        swap(first.stagingBuffer, second.stagingBuffer);
        swap(first.currentlyUtilized, second.currentlyUtilized);
        swap(first.pendingCopies, second.pendingCopies);
    }
} // namespace Prism::Resources