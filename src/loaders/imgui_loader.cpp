#include "loaders/imgui_loader.hpp"

#include "resources/vulkan_resource.hpp"
#include "resources/window_resource.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <stdexcept>

namespace Prism::Loaders {
    namespace {
        VkDescriptorPool createImGuiDescriptorPool(Resources::VulkanResource &vulkanResource) {
            VkDescriptorPool descriptorPool;
            VkDescriptorPoolSize poolSizes[] = {
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
            };

            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
            poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
            poolInfo.pPoolSizes = poolSizes;

            if (vkCreateDescriptorPool(vulkanResource.GetDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create descriptor pool!");
            }

            return descriptorPool;
        }

        VkRenderPass createImGuiRenderPass(Resources::VulkanResource &vulkanResource) {
            VkAttachmentDescription colorAttachment = {};
            colorAttachment.format = vulkanResource.GetSwapchainImageFormat();
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorAttachmentRef = {};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;

            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 1;
            renderPassInfo.pAttachments = &colorAttachment;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 0;
            renderPassInfo.pDependencies = nullptr;

            VkRenderPass renderPass;
            if (vkCreateRenderPass(vulkanResource.GetDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create render pass!");
            }

            return renderPass;
        }
    } // namespace

    ImGuiLoader::result_type ImGuiLoader::operator()(Resources::WindowResource &windowResource, Resources::VulkanResource &vulkanResource) const {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags = ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(windowResource.GetWindow(), true);

        auto descriptorPool = createImGuiDescriptorPool(vulkanResource);
        auto renderPass = createImGuiRenderPass(vulkanResource);

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = vulkanResource.GetInstance();
        initInfo.PhysicalDevice = vulkanResource.GetPhysicalDevice();
        initInfo.Device = vulkanResource.GetDevice();
        initInfo.Queue = vulkanResource.GetGraphicsQueue();
        initInfo.DescriptorPool = descriptorPool;
        initInfo.MinImageCount = vulkanResource.GetImageCount();
        initInfo.ImageCount = vulkanResource.GetImageCount();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.RenderPass = renderPass;

        ImGui_ImplVulkan_Init(&initInfo);

        return Resources::ImGuiResource(vulkanResource.GetDevice(), descriptorPool, renderPass);
    }
}; // namespace Prism::Loaders