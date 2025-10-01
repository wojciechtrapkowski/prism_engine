#include "systems/present_system.hpp"


#include <GLFW/glfw3.h>

namespace Prism::Systems {
    PresentSystem::PresentSystem(Resources::ContextResources &contextResources) : m_contextResources(contextResources) {}

    void PresentSystem::Initialize() {

    };

    void PresentSystem::Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene &scene) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        vkEndCommandBuffer(commandBuffer);
    };

    void PresentSystem::Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene &scene, Resources::RenderTargetResource &renderTarget) {
        {
            auto &vulkanResource = m_contextResources.GetVulkanResource();

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            VkImage srcImage = renderTarget.GetColorImage();
            VkImage dstImage = vulkanResource.GetRenderTargetImage();

            // Transition render target to transfer src
            VkImageMemoryBarrier barrierToTransferSrc{};
            barrierToTransferSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrierToTransferSrc.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrierToTransferSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrierToTransferSrc.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrierToTransferSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrierToTransferSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrierToTransferSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrierToTransferSrc.image = srcImage;
            barrierToTransferSrc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrierToTransferSrc.subresourceRange.baseMipLevel = 0;
            barrierToTransferSrc.subresourceRange.levelCount = 1;
            barrierToTransferSrc.subresourceRange.baseArrayLayer = 0;
            barrierToTransferSrc.subresourceRange.layerCount = 1;

            // Transition swapchain image to transfer dst
            VkImageMemoryBarrier barrierToTransferDst{};
            barrierToTransferDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrierToTransferDst.srcAccessMask = 0;
            barrierToTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrierToTransferDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrierToTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrierToTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrierToTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrierToTransferDst.image = dstImage;
            barrierToTransferDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrierToTransferDst.subresourceRange.baseMipLevel = 0;
            barrierToTransferDst.subresourceRange.levelCount = 1;
            barrierToTransferDst.subresourceRange.baseArrayLayer = 0;
            barrierToTransferDst.subresourceRange.layerCount = 1;

            VkImageMemoryBarrier barriers[] = {barrierToTransferSrc, barrierToTransferDst};
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2,
                                 barriers);

            // Copy the image
            VkImageCopy copyRegion{};
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.baseArrayLayer = 0;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.srcSubresource.mipLevel = 0;
            copyRegion.srcOffset = {0, 0, 0};

            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.baseArrayLayer = 0;
            copyRegion.dstSubresource.layerCount = 1;
            copyRegion.dstSubresource.mipLevel = 0;
            copyRegion.dstOffset = {0, 0, 0};

            copyRegion.extent.width = vulkanResource.GetSwapchainExtent().width;
            copyRegion.extent.height = vulkanResource.GetSwapchainExtent().height;
            copyRegion.extent.depth = 1;

            vkCmdCopyImage(commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            // Transition to present.
            VkImageMemoryBarrier barrierToPresent{};
            barrierToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrierToPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrierToPresent.dstAccessMask = 0;
            barrierToPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrierToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrierToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrierToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrierToPresent.image = dstImage;
            barrierToPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrierToPresent.subresourceRange.baseMipLevel = 0;
            barrierToPresent.subresourceRange.levelCount = 1;
            barrierToPresent.subresourceRange.baseArrayLayer = 0;
            barrierToPresent.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                 &barrierToPresent);

            // Transition render target back to color attachment optimal
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = renderTarget.GetColorImage();
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                 &barrier);

            vkEndCommandBuffer(commandBuffer);
        }
    }
} // namespace Prism::Systems