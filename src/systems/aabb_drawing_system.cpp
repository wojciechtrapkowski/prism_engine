#include "systems/aabb_drawing_system.hpp"

#include "glm/glm.hpp"

#include "assets/box.hpp"

#include "resources/mesh_resource.hpp"
#include "resources/common_resource.hpp"

#include "components/aabb.hpp"

#include "utils/vulkan/common.hpp"

#ifndef AABB_VERT_SHADER_PATH
#error "AABB_VERT_SHADER_PATH is not defined!"
#endif

#ifndef AABB_FRAG_SHADER_PATH
#error "AABB_FRAG_SHADER_PATH is not defined!"
#endif

namespace Prism::Systems
{
    namespace
    {
        VkDescriptorPool createDescriptorPool(VkDevice device)
        {
            VkDescriptorPool descriptorPool;

            std::array<VkDescriptorPoolSize, 2> poolSizes{};
            poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[0].descriptorCount = Resources::VulkanResource::FRAMES_IN_FLIGHT;

            poolSizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            poolSizes[1].descriptorCount = Resources::VulkanResource::FRAMES_IN_FLIGHT;

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
            uboBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
            uboBinding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutBinding transformBufferBinding{};
            transformBufferBinding.binding            = 1;
            transformBufferBinding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            transformBufferBinding.descriptorCount    = 1;
            transformBufferBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
            transformBufferBinding.pImmutableSamplers = nullptr;

            std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboBinding, transformBufferBinding};

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings    = bindings.data();

            VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
            bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;

            std::array<VkDescriptorBindingFlags, 2> bindingFlags = {0, 0};
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

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount         = 1;
            pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayout;
            pipelineLayoutInfo.pushConstantRangeCount = 0;
            pipelineLayoutInfo.pPushConstantRanges    = nullptr;

            if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create pipeline layout!");
            }

            return pipelineLayout;
        }

        VkPipeline createPipeline(VkDevice device, VkPipelineLayout pipelineLayout)
        {
            // Load shader modules
            VkShaderModule vertexShaderModule   = Utils::Vulkan::Common::loadShaderModule(device, AABB_VERT_SHADER_PATH);
            VkShaderModule fragmentShaderModule = Utils::Vulkan::Common::loadShaderModule(device, AABB_FRAG_SHADER_PATH);

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

            VkVertexInputBindingDescription binding{};
            binding.binding   = 0;
            binding.stride    = sizeof(Resources::MeshResource::Vertex);
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            VkVertexInputAttributeDescription attributes[3]{};
            attributes[0].binding  = 0;
            attributes[0].location = 0;
            attributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributes[0].offset   = offsetof(Resources::MeshResource::Vertex, position);

            attributes[1].binding  = 0;
            attributes[1].location = 1;
            attributes[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributes[1].offset   = offsetof(Resources::MeshResource::Vertex, normal);

            attributes[2].binding  = 0;
            attributes[2].location = 2;
            attributes[2].format   = VK_FORMAT_R32G32_SFLOAT;
            attributes[2].offset   = offsetof(Resources::MeshResource::Vertex, textureUV);

            vertexInputState.vertexBindingDescriptionCount   = 1;
            vertexInputState.pVertexBindingDescriptions      = &binding;
            vertexInputState.vertexAttributeDescriptionCount = 3;
            vertexInputState.pVertexAttributeDescriptions    = attributes;

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
            colorBlendAttachment.blendEnable    = VK_TRUE;

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

        void updateDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet, VkBuffer commonUniformBuffer, VkBuffer transformsBuffer)
        {
            VkDescriptorBufferInfo commonUniformBufferInfo{};
            commonUniformBufferInfo.buffer = commonUniformBuffer;
            commonUniformBufferInfo.offset = 0;
            commonUniformBufferInfo.range  = VK_WHOLE_SIZE;

            VkDescriptorBufferInfo transformsBufferInfo{};
            transformsBufferInfo.buffer = transformsBuffer;
            transformsBufferInfo.offset = 0;
            transformsBufferInfo.range  = VK_WHOLE_SIZE;

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

            VkWriteDescriptorSet transformDescriptorWrite{};
            transformDescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            transformDescriptorWrite.dstSet          = descriptorSet;
            transformDescriptorWrite.dstBinding      = 1;
            transformDescriptorWrite.dstArrayElement = 0;
            transformDescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            transformDescriptorWrite.descriptorCount = 1;
            transformDescriptorWrite.pBufferInfo     = &transformsBufferInfo;
            descriptorWrites.push_back(transformDescriptorWrite);

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        };
    } // namespace

    AABBDrawingSystem::AABBDrawingSystem(Resources::ContextResources& contextResources) : _contextResources(contextResources)
    {
        auto&    vulkanResource = _contextResources.GetVulkanResource();
        VkDevice device         = vulkanResource.GetDevice();

        _descriptorPool      = createDescriptorPool(device);
        _descriptorSetLayout = createDescriptorSetLayout(device);
        _descriptorSets      = createDescriptorSets(device, _descriptorPool, _descriptorSetLayout);
        _pipelineLayout      = createPipelineLayout(device, _descriptorSetLayout);
        _pipeline            = createPipeline(device, _pipelineLayout);
    };

    AABBDrawingSystem::~AABBDrawingSystem()
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

    void AABBDrawingSystem::Update(float deltaTime, VkCommandBuffer commandBuffer, Resources::VkStagingBufferResource& stagingBuffer, Resources::Scene& scene)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        _transformsBufferToDelete = std::nullopt;

        auto& vulkanResource = _contextResources.GetVulkanResource();
        auto& meshStorage    = scene.GetMeshStorage();
        auto& registry       = scene.GetRegistry();
        auto& systemsStorage = scene.GetSystemsStorage();

        auto& vmaAllocator = _contextResources.GetVulkanResource().GetVmaAllocator();

        auto aabbView = registry.view<Components::AABB>();
        if (aabbView.empty()) {
            vkEndCommandBuffer(commandBuffer);
            return;
        }

        auto boxMeshOpt = meshStorage.Get<Resources::MeshResource>(BOX_MESH_RESOURCE_ID);
        if (!boxMeshOpt) {
            auto boxVertices = Assets::Box::VERTICES;
            auto boxIndices  = Assets::Box::INDICES;

            std::vector<Resources::MeshResource::Vertex> transformedBoxVertices(boxVertices.size());
            for (size_t i = 0; i < transformedBoxVertices.size(); i++) {
                transformedBoxVertices[i].position  = boxVertices[i];
                transformedBoxVertices[i].normal    = glm::vec3{0.0f};
                transformedBoxVertices[i].textureUV = glm::vec2{0.0f};
            }

            std::vector<Resources::MeshResource::Index> transformedBoxIndices(boxIndices.size());
            for (size_t i = 0; i < transformedBoxIndices.size(); i++) {
                transformedBoxIndices[i].idx = boxIndices[i];
            }

            auto transformedBoxVerticesBufferSize = transformedBoxVertices.size() * sizeof(transformedBoxVertices[0]);
            Resources::VkBufferResource<Resources::MeshResource::Vertex> transformedBoxVerticesBuffer{
                vmaAllocator, transformedBoxVerticesBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT};

            stagingBuffer.Copy(transformedBoxVerticesBuffer.GetBuffer(), transformedBoxVertices.data(), transformedBoxVerticesBufferSize);

            auto transformedBoxIndicesBufferSize = transformedBoxVertices.size() * sizeof(transformedBoxVertices[0]);
            Resources::VkBufferResource<Resources::MeshResource::Index> transformedBoxIndicesBuffer{
                vmaAllocator, transformedBoxIndicesBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT};

            stagingBuffer.Copy(transformedBoxIndicesBuffer.GetBuffer(), transformedBoxIndices.data(), transformedBoxIndicesBufferSize);

            Resources::MeshResource boxMeshResource{
                "BoxMesh", std::move(transformedBoxVerticesBuffer), std::move(transformedBoxIndicesBuffer), transformedBoxVertices, transformedBoxIndices};

            meshStorage.Insert<Resources::MeshResource>(BOX_MESH_RESOURCE_ID, std::make_unique<Resources::MeshResource>(std::move(boxMeshResource)));
        }

        {
            bool rebuildTransformsBuffer = false;

            auto boxTransformsBufferOpt = systemsStorage.Get<Resources::VkBufferResource<BoxTransformEntry>>(BOX_TRANSFORMS_BUFFER_ID);

            rebuildTransformsBuffer |= !boxTransformsBufferOpt.has_value();

            if (boxTransformsBufferOpt) {
                auto& boxTransformsBuffer = boxTransformsBufferOpt->get();

                rebuildTransformsBuffer |= aabbView.size() != boxTransformsBuffer.GetElementCount();
            }

            if (rebuildTransformsBuffer) {
                if (boxTransformsBufferOpt) {
                    _transformsBufferToDelete = std::move(boxTransformsBufferOpt->get());
                    systemsStorage.Delete(BOX_TRANSFORMS_BUFFER_ID);
                }

                auto boxTransformsBufferSize = aabbView.size() * sizeof(BoxTransformEntry);

                Resources::VkBufferResource<BoxTransformEntry> boxTransformsBuffer{
                    vmaAllocator, boxTransformsBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU};

                systemsStorage.Insert(
                    BOX_TRANSFORMS_BUFFER_ID, std::make_unique<Resources::VkBufferResource<BoxTransformEntry>>(std::move(boxTransformsBuffer)));
            }

            boxTransformsBufferOpt = systemsStorage.Get<Resources::VkBufferResource<BoxTransformEntry>>(BOX_TRANSFORMS_BUFFER_ID);

            auto& boxTransformsBuffer = boxTransformsBufferOpt->get();

            std::vector<BoxTransformEntry> boxTransforms;
            boxTransforms.reserve(aabbView.size());
            for (auto&& [entity, aabbComponent] : aabbView.each()) {
                BoxTransformEntry entry;
                entry.entityId = static_cast<uint32_t>(entity);
                entry.lower    = aabbComponent.lower;
                entry.upper    = glm::vec3(aabbComponent.upper);
                boxTransforms.push_back(std::move(entry));
            }

            auto boxTransformsBufferSize = boxTransforms.size() * sizeof(boxTransforms[0]);

            void* data = nullptr;
            if (vmaMapMemory(vulkanResource.GetVmaAllocator(), boxTransformsBuffer.GetAllocation(), &data) == VK_SUCCESS) {
                std::memcpy(data, boxTransforms.data(), boxTransformsBufferSize);
                vmaUnmapMemory(vulkanResource.GetVmaAllocator(), boxTransformsBuffer.GetAllocation());
            }
        }

        vkEndCommandBuffer(commandBuffer);
    }

    void AABBDrawingSystem::Render(float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget)
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

        auto boxMeshOpt = meshStorage.Get<Resources::MeshResource>(BOX_MESH_RESOURCE_ID);
        if (!boxMeshOpt) {
            vkEndCommandBuffer(commandBuffer);
            return;
        }
        auto& boxMesh = boxMeshOpt->get();

        auto commonUniformBufferOpt =
            resourceStorage.Get<Resources::VkBufferResource<Resources::CommonResource>>(Resources::CommonResource::UNIFORM_BUFFER_ID, currentFrame);
        if (!commonUniformBufferOpt) {
            vkEndCommandBuffer(commandBuffer);
            return;
        }
        auto& commonUniformBuffer = commonUniformBufferOpt->get();

        auto transformsBufferOpt = systemsStorage.Get<Resources::VkBufferResource<BoxTransformEntry>>(BOX_TRANSFORMS_BUFFER_ID);
        if (!transformsBufferOpt) {
            vkEndCommandBuffer(commandBuffer);
            return;
        }
        auto& transformsBuffer = transformsBufferOpt->get();

        updateDescriptorSet(vulkanResource.GetDevice(), _descriptorSets[currentFrame], commonUniformBuffer.GetBuffer(), transformsBuffer.GetBuffer());

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

        VkBuffer     vertexBuffers[] = {boxMesh.GetVertexBuffer().GetBuffer()};
        VkDeviceSize offsets[]       = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, boxMesh.GetIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        auto elCount = transformsBuffer.GetElementCount();
        vkCmdDrawIndexed(commandBuffer, boxMesh.GetIndexBuffer().GetElementCount(), transformsBuffer.GetElementCount(), 0, 0, 0);

        vkCmdEndRendering(commandBuffer);

        vkEndCommandBuffer(commandBuffer);
    }
} // namespace Prism::Systems