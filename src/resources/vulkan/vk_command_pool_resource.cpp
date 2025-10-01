#include "resources/vulkan/vk_command_pool_resource.hpp"

#include <stdexcept>

namespace Prism::Resources {

    VkCommandPoolResource::VkCommandPoolResource(VkDevice device, uint32_t queueFamilyIndex) : device(device) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndex;

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }

        commandBuffers.resize(INITIAL_BUFFER_COUNT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = INITIAL_BUFFER_COUNT;

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    VkCommandPoolResource::~VkCommandPoolResource() {
        if (device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device);
        }

        if (commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }
    }

    VkCommandPoolResource::VkCommandPoolResource(VkCommandPoolResource &&other) noexcept { swap(*this, other); }

    VkCommandPoolResource &VkCommandPoolResource::operator=(VkCommandPoolResource &&other) noexcept {
        swap(*this, other);
        return *this;
    }

    VkCommandPoolResource::Scope VkCommandPoolResource::BeginScope() { return Scope(*this, currentBufferIndex); }

    void VkCommandPoolResource::Reset() {
        if (vkResetCommandPool(device, commandPool, 0) != VK_SUCCESS) {
            throw std::runtime_error("Failed to reset command pool!");
        }

        currentBufferIndex = 0;
    }

    void swap(VkCommandPoolResource &lhs, VkCommandPoolResource &rhs) noexcept {
        using std::swap;
        swap(lhs.device, rhs.device);
        swap(lhs.commandPool, rhs.commandPool);
        swap(lhs.commandBuffers, rhs.commandBuffers);
        swap(lhs.currentBufferIndex, rhs.currentBufferIndex);
    }

    VkCommandBuffer VkCommandPoolResource::GetNextCommandBuffer() {
        if (currentBufferIndex >= commandBuffers.size()) {
            // Allocate twice what we have.
            uint32_t additionalCount = static_cast<uint32_t>(commandBuffers.size());

            std::vector<VkCommandBuffer> newBuffers(additionalCount);

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = additionalCount;

            if (vkAllocateCommandBuffers(device, &allocInfo, newBuffers.data()) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate extra command buffers!");
            }

            commandBuffers.insert(commandBuffers.end(), newBuffers.begin(), newBuffers.end());
        }

        return commandBuffers[currentBufferIndex++];
    }

    // Scope implementation
    VkCommandPoolResource::Scope::Scope(VkCommandPoolResource &parent, size_t startIndex) : parent(parent), startIndex(startIndex), localIndex(0) {}

    VkCommandBuffer VkCommandPoolResource::Scope::GetNextCommandBuffer() {
        localIndex++;
        return parent.GetNextCommandBuffer();
    }

    void VkCommandPoolResource::Scope::Reset() {
        for (size_t i = 0; i < localIndex; ++i) {
            vkResetCommandBuffer(parent.commandBuffers[startIndex + i], 0);
        }
        localIndex = 0;
    }

    size_t VkCommandPoolResource::Scope::size() const noexcept { return localIndex; }

    VkCommandBuffer *VkCommandPoolResource::Scope::data() noexcept { return parent.commandBuffers.data() + startIndex; }

    const VkCommandBuffer *VkCommandPoolResource::Scope::data() const noexcept { return parent.commandBuffers.data() + startIndex; }

    VkCommandBuffer &VkCommandPoolResource::Scope::operator[](size_t idx) {
        if (idx >= localIndex) {
            throw std::out_of_range("Scope index out of range");
        }
        return parent.commandBuffers[startIndex + idx];
    }

    const VkCommandBuffer &VkCommandPoolResource::Scope::operator[](size_t idx) const {
        if (idx >= localIndex) {
            throw std::out_of_range("Scope index out of range");
        }
        return parent.commandBuffers[startIndex + idx];
    }
} // namespace Prism::Resources