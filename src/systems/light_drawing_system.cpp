#include "systems/light_drawing_system.hpp"

#include "glm/glm.hpp"

#include "resources/common_resource.hpp"

#include "components/light.hpp"
#include "components/transform.hpp"

#include "utils/vulkan/common.hpp"

#ifndef PASS_THROUGH_VERT_SHADER_PATH
#error "PASS_THROUGH_VERT_SHADER_PATH is not defined!"
#endif

#ifndef LIGHTS_FRAG_SHADER_PATH
#error "LIGHTS_FRAG_SHADER_PATH is not defined!"
#endif

namespace Prism::Systems
{
    namespace
    {
        VkDescriptorPool createDescriptorPool(VkDevice device)
        {
            VkDescriptorPool descriptorPool;

            std::array<VkDescriptorPoolSize, 1> poolSizes{};
            poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[0].descriptorCount = Resources::VulkanResource::FRAMES_IN_FLIGHT;

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.maxSets       = Resources::VulkanResource::FRAMES_IN_FLIGHT;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes    = poolSizes.data();

            if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create descriptor pool!");
            }

            return descriptorPool;
        }

        VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device)
        {
            VkDescriptorSetLayout descriptorSetLayout;

            VkDescriptorSetLayoutBinding uboBinding{};
            uboBinding.binding            = 0;
            uboBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboBinding.descriptorCount    = 1;
            uboBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
            uboBinding.pImmutableSamplers = nullptr;

            std::array<VkDescriptorSetLayoutBinding, 1> bindings = {uboBinding};

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings    = bindings.data();

            VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
            bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;

            std::array<VkDescriptorBindingFlags, 1> bindingFlags = {0};
            bindingFlagsInfo.bindingCount                        = static_cast<uint32_t>(bindingFlags.size());
            bindingFlagsInfo.pBindingFlags                       = bindingFlags.data();

            layoutInfo.pNext = &bindingFlagsInfo;

            if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout");
            }

            return descriptorSetLayout;
        }

        std::vector<VkDescriptorSet> createDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
        {
            std::vector<VkDescriptorSet> descriptorSets;

            descriptorSets.resize(Resources::VulkanResource::FRAMES_IN_FLIGHT);

            std::vector<VkDescriptorSetLayout> layouts(Resources::VulkanResource::FRAMES_IN_FLIGHT, descriptorSetLayout);

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool     = descriptorPool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
            allocInfo.pSetLayouts        = layouts.data();

            if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate descriptor sets!");
            }

            return descriptorSets;
        }

        VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
        {
            VkPipelineLayout pipelineLayout;

            VkPushConstantRange fPushConstantsRange;
            fPushConstantsRange.offset     = 0;
            fPushConstantsRange.size       = sizeof(LightDrawingSystem::FragmentShaderPushConstants);
            fPushConstantsRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount         = 1;
            pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayout;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges    = &fPushConstantsRange;

            if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create pipeline layout!");
            }

            return pipelineLayout;
        }

        VkPipeline createPipeline(VkDevice device, VkPipelineLayout pipelineLayout)
        {
            // Load shader modules
            VkShaderModule vertexShaderModule   = Utils::Vulkan::Common::loadShaderModule(device, PASS_THROUGH_VERT_SHADER_PATH);
            VkShaderModule fragmentShaderModule = Utils::Vulkan::Common::loadShaderModule(device, LIGHTS_FRAG_SHADER_PATH);

            // Shader stages
            VkPipelineShaderStageCreateInfo shaderStages[2]{};

            shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
            shaderStages[0].module = vertexShaderModule;
            shaderStages[0].pName  = "main";

            shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderStages[1].module = fragmentShaderModule;
            shaderStages[1].pName  = "main";

            // Vertex input state
            VkPipelineVertexInputStateCreateInfo vertexInputState{};
            vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            vertexInputState.vertexBindingDescriptionCount   = 0;
            vertexInputState.pVertexBindingDescriptions      = nullptr;
            vertexInputState.vertexAttributeDescriptionCount = 0;
            vertexInputState.pVertexAttributeDescriptions    = nullptr;

            // Input assembly
            VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
            inputAssemblyState.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            // Viewport and scissor state
            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount  = 1;

            // Rasterizer
            VkPipelineRasterizationStateCreateInfo rasterizationState{};
            rasterizationState.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizationState.cullMode    = VK_CULL_MODE_NONE;
            rasterizationState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizationState.lineWidth   = 1.0f;

            // Multisampling
            VkPipelineMultisampleStateCreateInfo multisampleState{};
            multisampleState.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            // Depth and stencil state
            VkPipelineDepthStencilStateCreateInfo depthStencilState{};
            depthStencilState.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencilState.depthTestEnable  = VK_TRUE;
            depthStencilState.depthWriteEnable = VK_TRUE;
            depthStencilState.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

            // Color blend attachment
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable    = VK_FALSE;

            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

            // Color blend state
            VkPipelineColorBlendStateCreateInfo colorBlendState{};
            colorBlendState.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendState.attachmentCount = 1;
            colorBlendState.pAttachments    = &colorBlendAttachment;

            // Dynamic states
            VkDynamicState                   dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
            dynamicStateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(std::size(dynamicStates));
            dynamicStateInfo.pDynamicStates    = dynamicStates;

            // Dynamic rendering info (no render pass)
            VkFormat colorFormat = Resources::RenderTargetResource::GetColorFormat();

            VkPipelineRenderingCreateInfo renderingInfo{};
            renderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
            renderingInfo.colorAttachmentCount    = 1;
            renderingInfo.pColorAttachmentFormats = &colorFormat;
            renderingInfo.depthAttachmentFormat   = Resources::RenderTargetResource::GetDepthFormat();
            renderingInfo.stencilAttachmentFormat = Resources::RenderTargetResource::GetDepthFormat();

            // Graphics pipeline create info
            VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
            pipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineCreateInfo.pNext               = &renderingInfo;
            pipelineCreateInfo.stageCount          = 2;
            pipelineCreateInfo.pStages             = shaderStages;
            pipelineCreateInfo.pVertexInputState   = &vertexInputState;
            pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
            pipelineCreateInfo.pViewportState      = &viewportState;
            pipelineCreateInfo.pRasterizationState = &rasterizationState;
            pipelineCreateInfo.pMultisampleState   = &multisampleState;
            pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
            pipelineCreateInfo.pColorBlendState    = &colorBlendState;
            pipelineCreateInfo.pDynamicState       = &dynamicStateInfo;
            pipelineCreateInfo.layout              = pipelineLayout;
            pipelineCreateInfo.renderPass          = VK_NULL_HANDLE; // dynamic rendering
            pipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;

            // Create graphics pipeline
            VkPipeline pipeline{};
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
                vkDestroyShaderModule(device, vertexShaderModule, nullptr);
                vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
                throw std::runtime_error("vkCreateGraphicsPipelines failed");
            }

            // Cleanup shader modules
            vkDestroyShaderModule(device, vertexShaderModule, nullptr);
            vkDestroyShaderModule(device, fragmentShaderModule, nullptr);

            return pipeline;
        }

        void updateDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet, VkBuffer commonUniformBuffer)
        {
            VkDescriptorBufferInfo commonUniformBufferInfo{};
            commonUniformBufferInfo.buffer = commonUniformBuffer;
            commonUniformBufferInfo.offset = 0;
            commonUniformBufferInfo.range  = VK_WHOLE_SIZE;

            std::vector<VkWriteDescriptorSet> descriptorWrites = {};

            VkWriteDescriptorSet commonUniformDescriptorWrite{};
            commonUniformDescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            commonUniformDescriptorWrite.dstSet          = descriptorSet;
            commonUniformDescriptorWrite.dstBinding      = 0;
            commonUniformDescriptorWrite.dstArrayElement = 0;
            commonUniformDescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            commonUniformDescriptorWrite.descriptorCount = 1;
            commonUniformDescriptorWrite.pBufferInfo     = &commonUniformBufferInfo;
            descriptorWrites.push_back(commonUniformDescriptorWrite);

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        };
    } // namespace

    LightDrawingSystem::LightDrawingSystem(Resources::ContextResources& contextResources) : _contextResources(contextResources)
    {
        auto&    vulkanResource = _contextResources.GetVulkanResource();
        VkDevice device         = vulkanResource.GetDevice();

        _descriptorPool      = createDescriptorPool(device);
        _descriptorSetLayout = createDescriptorSetLayout(device);
        _descriptorSets      = createDescriptorSets(device, _descriptorPool, _descriptorSetLayout);
        _pipelineLayout      = createPipelineLayout(device, _descriptorSetLayout);
        _pipeline            = createPipeline(device, _pipelineLayout);
    };

    LightDrawingSystem::~LightDrawingSystem()
    {
        auto&    vulkanResource = _contextResources.GetVulkanResource();
        VkDevice device         = vulkanResource.GetDevice();

        if (_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, _pipeline, nullptr);
            _pipeline = VK_NULL_HANDLE;
        }
        if (_pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
            _pipelineLayout = VK_NULL_HANDLE;
        }
        if (_descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, _descriptorSetLayout, nullptr);
            _descriptorSetLayout = VK_NULL_HANDLE;
        }
        if (_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
            _descriptorPool = VK_NULL_HANDLE;
        }
    }

    void LightDrawingSystem::Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::VkStagingBufferResource& stagingBuffer, Resources::Scene& scene)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        vkEndCommandBuffer(commandBuffer);
    }

    void LightDrawingSystem::Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        auto& registry       = scene.GetRegistry();
        auto& meshStorage    = scene.GetMeshStorage();
        auto& systemsStorage = scene.GetSystemsStorage();

        auto& resourceStorage = _contextResources.GetResourceStorage();
        auto& vulkanResource  = _contextResources.GetVulkanResource();

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView   = renderTarget.GetColorImageView();
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView   = renderTarget.GetDepthImageView();
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

        auto currentSwapchainExtent = vulkanResource.GetSwapchainExtent();

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset    = {0, 0};
        renderingInfo.renderArea.extent    = {currentSwapchainExtent.width, currentSwapchainExtent.height};
        renderingInfo.layerCount           = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments    = &colorAttachment;
        renderingInfo.pDepthAttachment     = &depthAttachment;

        auto currentFrame = vulkanResource.GetCurrentFrameOffset();

        auto commonUniformBufferOpt =
            resourceStorage.Get<Resources::VkBufferResource<Resources::CommonResource>>(Resources::CommonResource::UNIFORM_BUFFER_ID, currentFrame);
        if (!commonUniformBufferOpt) {
            vkEndCommandBuffer(commandBuffer);
            return;
        }
        auto& commonUniformBuffer = commonUniformBufferOpt->get();

        updateDescriptorSet(vulkanResource.GetDevice(), _descriptorSets[currentFrame], commonUniformBuffer.GetBuffer());

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(currentSwapchainExtent.width);
        viewport.height   = static_cast<float>(currentSwapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {currentSwapchainExtent.width, currentSwapchainExtent.height};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSets[currentFrame], 0, nullptr);

        auto lightsView = registry.view<Components::Light, Components::Transform>();
        for (auto&& [entity, light, transform] : lightsView.each()) {
            FragmentShaderPushConstants pc{};
            pc.position = glm::vec3(transform.transform[3]);
            pc.strength = light.strength;

            vkCmdPushConstants(commandBuffer, _pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FragmentShaderPushConstants), &pc);

            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        }

        vkCmdEndRendering(commandBuffer);

        vkEndCommandBuffer(commandBuffer);
    }
} // namespace Prism::Systems