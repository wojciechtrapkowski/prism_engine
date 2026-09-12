#include "systems/subsystems/mesh_drawing_system/raytraced_geometry_drawing_subsystem.hpp"

#include "components/mesh.hpp"
#include "components/transform.hpp"

#include "utils/vulkan/common.hpp"

#include "resources/common_resource.hpp"
#include "resources/vulkan/vk_buffer_resource.hpp"
#include "resources/vulkan/vk_acceleration_structure_resource.hpp"

#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <iostream>
#include <vector>
#include <cstring>

#include "volk/volk.h"

#ifndef RAY_RGEN_SHADER_PATH
#error "RAY_RGEN_SHADER_PATH is not defined!"
#endif

#ifndef RAY_RMISS_SHADER_PATH
#error "RAY_RMISS_SHADER_PATH is not defined!"
#endif

#ifndef RAY_RCHIT_SHADER_PATH
#error "RAY_RCHIT_SHADER_PATH is not defined!"
#endif

namespace Prism::Systems::Subsystems::MeshDrawingSystem
{
    namespace
    {
        inline static const uint32_t MAX_NUMBER_OF_TEXTURES = 1024;

        VkDescriptorPool createDescriptorPool(VkDevice device)
        {
            VkDescriptorPool descriptorPool;

            std::array<VkDescriptorPoolSize, 5> poolSizes{};
            poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[0].descriptorCount = Resources::VulkanResource::FRAMES_IN_FLIGHT;

            poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSizes[1].descriptorCount = Resources::VulkanResource::FRAMES_IN_FLIGHT * MAX_NUMBER_OF_TEXTURES;

            poolSizes[2].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            poolSizes[2].descriptorCount = Resources::VulkanResource::FRAMES_IN_FLIGHT;

            poolSizes[3].type            = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            poolSizes[3].descriptorCount = Resources::VulkanResource::FRAMES_IN_FLIGHT;

            poolSizes[4].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            poolSizes[4].descriptorCount = Resources::VulkanResource::FRAMES_IN_FLIGHT;

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

            // Binding 0: common uniform buffer (view, projection, camera position)
            VkDescriptorSetLayoutBinding uboBinding{};
            uboBinding.binding            = 0;
            uboBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboBinding.descriptorCount    = 1;
            uboBinding.stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            uboBinding.pImmutableSamplers = nullptr;

            // Binding 1: textures
            VkDescriptorSetLayoutBinding samplerBinding{};
            samplerBinding.binding            = 1;
            samplerBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerBinding.descriptorCount    = MAX_NUMBER_OF_TEXTURES;
            samplerBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            samplerBinding.pImmutableSamplers = nullptr;

            // Binding 2: meshes infos
            VkDescriptorSetLayoutBinding meshesInfosBinding{};
            meshesInfosBinding.binding            = 2;
            meshesInfosBinding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            meshesInfosBinding.descriptorCount    = 1;
            meshesInfosBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            meshesInfosBinding.pImmutableSamplers = nullptr;

            // Binding 3: top-level acceleration structure
            VkDescriptorSetLayoutBinding tlasBinding{};
            tlasBinding.binding            = 3;
            tlasBinding.descriptorType     = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            tlasBinding.descriptorCount    = 1;
            tlasBinding.stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            tlasBinding.pImmutableSamplers = nullptr;

            // Binding 4: storage image for ray tracing output
            VkDescriptorSetLayoutBinding outputImageBinding{};
            outputImageBinding.binding            = 4;
            outputImageBinding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            outputImageBinding.descriptorCount    = 1;
            outputImageBinding.stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            outputImageBinding.pImmutableSamplers = nullptr;

            std::array<VkDescriptorSetLayoutBinding, 5> bindings = {uboBinding, samplerBinding, meshesInfosBinding, tlasBinding, outputImageBinding};

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings    = bindings.data();

            VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
            bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;

            std::array<VkDescriptorBindingFlags, 5> bindingFlags = {
                0,                                         // binding 0 (UBO) - no special flags
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT, // binding 1 (samplers) - we won't use all of them.
                0,                                         // binding 2 (meshes infos) - no special flags
                0,                                         // binding 3 (TLAS) - no special flags
                0                                          // binding 4 (output image) - no special flags
            };

            bindingFlagsInfo.bindingCount  = static_cast<uint32_t>(bindingFlags.size());
            bindingFlagsInfo.pBindingFlags = bindingFlags.data();

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

        VkPipeline createPipeline(VkDevice device, VkPhysicalDevice physicalDevice, VkPipelineLayout pipelineLayout)
        {
            // Load shader modules
            VkShaderModule rgenModule  = Utils::Vulkan::Common::loadShaderModule(device, RAY_RGEN_SHADER_PATH);
            VkShaderModule rmissModule = Utils::Vulkan::Common::loadShaderModule(device, RAY_RMISS_SHADER_PATH);
            VkShaderModule rchitModule = Utils::Vulkan::Common::loadShaderModule(device, RAY_RCHIT_SHADER_PATH);

            // Shader stages: 0 = raygen, 1 = miss, 2 = closest-hit
            std::array<VkPipelineShaderStageCreateInfo, 3> stages{};
            stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_RAYGEN_BIT_KHR, rgenModule, "main"};
            stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_MISS_BIT_KHR, rmissModule, "main"};
            stages[2] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, rchitModule, "main"};

            // Shader groups: raygen (general), miss (general), triangle hit group (closest-hit)
            std::array<VkRayTracingShaderGroupCreateInfoKHR, 3> groups{};

            groups[0].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            groups[0].type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            groups[0].generalShader      = 0; // raygen
            groups[0].closestHitShader   = VK_SHADER_UNUSED_KHR;
            groups[0].anyHitShader       = VK_SHADER_UNUSED_KHR;
            groups[0].intersectionShader = VK_SHADER_UNUSED_KHR;

            groups[1].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            groups[1].type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            groups[1].generalShader      = 1; // miss
            groups[1].closestHitShader   = VK_SHADER_UNUSED_KHR;
            groups[1].anyHitShader       = VK_SHADER_UNUSED_KHR;
            groups[1].intersectionShader = VK_SHADER_UNUSED_KHR;

            groups[2].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            groups[2].type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            groups[2].generalShader      = VK_SHADER_UNUSED_KHR;
            groups[2].closestHitShader   = 2; // closest-hit
            groups[2].anyHitShader       = VK_SHADER_UNUSED_KHR;
            groups[2].intersectionShader = VK_SHADER_UNUSED_KHR;

            VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo{};
            pipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
            pipelineCreateInfo.stageCount                   = static_cast<uint32_t>(stages.size());
            pipelineCreateInfo.pStages                      = stages.data();
            pipelineCreateInfo.groupCount                   = static_cast<uint32_t>(groups.size());
            pipelineCreateInfo.pGroups                      = groups.data();
            pipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
            pipelineCreateInfo.layout                       = pipelineLayout;

            VkPipeline pipeline{};
            if (vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
                vkDestroyShaderModule(device, rgenModule, nullptr);
                vkDestroyShaderModule(device, rmissModule, nullptr);
                vkDestroyShaderModule(device, rchitModule, nullptr);
                throw std::runtime_error("vkCreateRayTracingPipelinesKHR failed");
            }

            vkDestroyShaderModule(device, rgenModule, nullptr);
            vkDestroyShaderModule(device, rmissModule, nullptr);
            vkDestroyShaderModule(device, rchitModule, nullptr);

            return pipeline;
        }

        void updateDescriptorSet(
            VkDevice                            device,
            VkDescriptorSet                     descriptorSet,
            VkBuffer                            commonUniformBuffer,
            std::vector<VkDescriptorImageInfo>& textureImageInfos,
            VkBuffer                            meshesInfos,
            VkAccelerationStructureKHR          tlas,
            VkImageView                         outputImageView)
        {
            std::vector<VkWriteDescriptorSet> descriptorWrites = {};

            // Binding 0: common UBO
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = commonUniformBuffer;
            bufferInfo.offset = 0;
            bufferInfo.range  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet uboWrite{};
            uboWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uboWrite.dstSet          = descriptorSet;
            uboWrite.dstBinding      = 0;
            uboWrite.dstArrayElement = 0;
            uboWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWrite.descriptorCount = 1;
            uboWrite.pBufferInfo     = &bufferInfo;

            descriptorWrites.push_back(uboWrite);

            // Binding 1: Textures
            if (!textureImageInfos.empty()) {
                VkWriteDescriptorSet samplerWrite{};
                samplerWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                samplerWrite.dstSet          = descriptorSet;
                samplerWrite.dstBinding      = 1;
                samplerWrite.dstArrayElement = 0;
                samplerWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                samplerWrite.descriptorCount = static_cast<uint32_t>(textureImageInfos.size());
                samplerWrite.pImageInfo      = textureImageInfos.data();
                descriptorWrites.push_back(samplerWrite);
            }

            // Binding 2: Meshes infos
            VkDescriptorBufferInfo meshesInfosBufferInfo{};
            meshesInfosBufferInfo.buffer = meshesInfos;
            meshesInfosBufferInfo.offset = 0;
            meshesInfosBufferInfo.range  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet meshesInfosWrite{};
            meshesInfosWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            meshesInfosWrite.dstSet          = descriptorSet;
            meshesInfosWrite.dstBinding      = 2;
            meshesInfosWrite.dstArrayElement = 0;
            meshesInfosWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            meshesInfosWrite.descriptorCount = 1;
            meshesInfosWrite.pBufferInfo     = &meshesInfosBufferInfo;
            descriptorWrites.push_back(meshesInfosWrite);

            // Binding 3: TLAS
            VkWriteDescriptorSetAccelerationStructureKHR tlasWriteAS{};
            tlasWriteAS.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
            tlasWriteAS.accelerationStructureCount = 1;
            tlasWriteAS.pAccelerationStructures    = &tlas;

            VkWriteDescriptorSet tlasWrite{};
            tlasWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            tlasWrite.pNext           = &tlasWriteAS;
            tlasWrite.dstSet          = descriptorSet;
            tlasWrite.dstBinding      = 3;
            tlasWrite.dstArrayElement = 0;
            tlasWrite.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            tlasWrite.descriptorCount = 1;

            descriptorWrites.push_back(tlasWrite);

            // Binding 4: storage image (output)
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageView   = outputImageView;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkWriteDescriptorSet imageWrite{};
            imageWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageWrite.dstSet          = descriptorSet;
            imageWrite.dstBinding      = 4;
            imageWrite.dstArrayElement = 0;
            imageWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            imageWrite.descriptorCount = 1;
            imageWrite.pImageInfo      = &imageInfo;

            descriptorWrites.push_back(imageWrite);

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        };

        Resources::VkAccelerationStructureResource createBLASFromMesh(
            Resources::ContextResources& contextResources,
            VkCommandBuffer              commandBuffer,
            Resources::MeshResource&     mesh,
            Resources::Resource::ID      scratchBufferId)
        {
            auto& vulkan                   = contextResources.GetVulkanResource();
            auto& temporaryResourceStorage = contextResources.GetTemporaryResourceStorage();

            VkAccelerationStructureKHR accelStruct{};

            // Prepare geometry

            const auto triangleCount = static_cast<uint32_t>(mesh.GetIndexBuffer().GetElementCount() / 3U);

            auto& vertexBuffer = mesh.GetVertexBuffer();
            auto& indexBuffer  = mesh.GetIndexBuffer();

            VkAccelerationStructureGeometryTrianglesDataKHR accelTriangles{
                .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat = mesh.GetVertexType(),
                .vertexData   = {.deviceAddress = vertexBuffer.GetBufferDeviceAddress()},
                .vertexStride = vertexBuffer.GetElementSize(),
                .maxVertex    = static_cast<uint32_t>(vertexBuffer.GetElementCount()),
                .indexType    = mesh.GetIndexType(),
                .indexData    = {.deviceAddress = indexBuffer.GetBufferDeviceAddress()},
            };

            VkAccelerationStructureGeometryKHR accelGeometry = VkAccelerationStructureGeometryKHR{
                .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
                .geometry     = {.triangles = accelTriangles},
                .flags        = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR | VK_GEOMETRY_OPAQUE_BIT_KHR,
            };

            VkAccelerationStructureBuildRangeInfoKHR accelRangeInfo{.primitiveCount = triangleCount};

            VkAccelerationStructureBuildGeometryInfoKHR accelBuildInfo{
                .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                .type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                .flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
                .mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                .geometryCount = 1,
                .pGeometries   = &accelGeometry,
            };

            std::vector<uint32_t> maxPrimCount(1);
            maxPrimCount[0] = accelRangeInfo.primitiveCount;

            // Create buffers for acceleration structure and scratch space
            VkAccelerationStructureBuildSizesInfoKHR accelBuildSize{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
            vkGetAccelerationStructureBuildSizesKHR(
                vulkan.GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelBuildInfo, maxPrimCount.data(), &accelBuildSize);

            Resources::VkBufferResource<> accelerationStructureBuffer{
                vulkan.GetVmaAllocator(),
                accelBuildSize.accelerationStructureSize,
                VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT};

            VkPhysicalDeviceAccelerationStructurePropertiesKHR accelProps{};
            accelProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
            VkPhysicalDeviceProperties2 props2{};
            props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            props2.pNext = &accelProps;
            vkGetPhysicalDeviceProperties2(vulkan.GetPhysicalDevice(), &props2);

            Resources::VkBufferResource<> scratchBuffer{
                vulkan.GetVmaAllocator(),
                accelBuildSize.buildScratchSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                accelProps.minAccelerationStructureScratchOffsetAlignment};

            VkAccelerationStructureCreateInfoKHR accelCreateInfo{
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
                .size  = accelBuildSize.accelerationStructureSize,
                .type  = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            };

            accelCreateInfo.buffer = accelerationStructureBuffer.GetBuffer();

            // Create and build acceleration structure

            if (vkCreateAccelerationStructureKHR(vulkan.GetDevice(), &accelCreateInfo, nullptr, &accelStruct) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create acceleration structure!");
            }

            accelBuildInfo.dstAccelerationStructure  = accelStruct;
            accelBuildInfo.scratchData.deviceAddress = scratchBuffer.GetBufferDeviceAddress();

            VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = &accelRangeInfo;

            vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelBuildInfo, &pBuildRangeInfo);

            auto meshId = mesh.GetID();

            temporaryResourceStorage.Insert(scratchBufferId, std::make_unique<Resources::VkBufferResource<>>(std::move(scratchBuffer)));

            return {vulkan.GetDevice(), std::move(accelerationStructureBuffer), std::move(accelStruct)};
        }

        Resources::VkAccelerationStructureResource createTLAS(
            Resources::ContextResources&                                     contextResources,
            VkCommandBuffer                                                  commandBuffer,
            Resources::VkBufferResource<VkAccelerationStructureInstanceKHR>& tlasInstancesBuffer,
            std::vector<VkAccelerationStructureInstanceKHR>&                 instances,
            Resources::Resource::ID                                          scratchBufferId)
        {
            auto& vulkan          = contextResources.GetVulkanResource();
            auto& resourceStorage = contextResources.GetResourceStorage();

            VkAccelerationStructureGeometryInstancesDataKHR geometryInstances{
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                .data  = {.deviceAddress = tlasInstancesBuffer.GetBufferDeviceAddress()}};

            VkAccelerationStructureGeometryKHR accelGeometry = {
                .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
                .geometry     = {.instances = geometryInstances}};

            VkAccelerationStructureBuildRangeInfoKHR accelBuildRangeInfo = {.primitiveCount = static_cast<uint32_t>(instances.size())};

            VkAccelerationStructureKHR accelStruct{};

            // Prepare geometry

            VkAccelerationStructureBuildGeometryInfoKHR accelBuildInfo{
                .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                .type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
                .flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
                .mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                .geometryCount = 1,
                .pGeometries   = &accelGeometry,
            };

            std::vector<uint32_t> maxPrimCount(1);
            maxPrimCount[0] = accelBuildRangeInfo.primitiveCount;

            // Create buffers for acceleration structure and scratch space
            VkAccelerationStructureBuildSizesInfoKHR accelBuildSize{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
            vkGetAccelerationStructureBuildSizesKHR(
                vulkan.GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelBuildInfo, maxPrimCount.data(), &accelBuildSize);

            Resources::VkBufferResource<> accelerationStructureBuffer{
                vulkan.GetVmaAllocator(),
                accelBuildSize.accelerationStructureSize,
                VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT};

            VkPhysicalDeviceAccelerationStructurePropertiesKHR accelProps{};
            accelProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
            VkPhysicalDeviceProperties2 props2{};
            props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            props2.pNext = &accelProps;
            vkGetPhysicalDeviceProperties2(vulkan.GetPhysicalDevice(), &props2);

            Resources::VkBufferResource<> scratchBuffer{
                vulkan.GetVmaAllocator(),
                accelBuildSize.buildScratchSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                accelProps.minAccelerationStructureScratchOffsetAlignment};

            VkAccelerationStructureCreateInfoKHR accelCreateInfo{
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
                .size  = accelBuildSize.accelerationStructureSize,
                .type  = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
            };

            accelCreateInfo.buffer = accelerationStructureBuffer.GetBuffer();

            // Create and build acceleration structure

            if (vkCreateAccelerationStructureKHR(vulkan.GetDevice(), &accelCreateInfo, nullptr, &accelStruct) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create acceleration structure!");
            }

            accelBuildInfo.dstAccelerationStructure  = accelStruct;
            accelBuildInfo.scratchData.deviceAddress = scratchBuffer.GetBufferDeviceAddress();

            VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = &accelBuildRangeInfo;

            vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelBuildInfo, &pBuildRangeInfo);

            // This will be reused for refitting the TLAS.
            resourceStorage.Insert(scratchBufferId, std::make_unique<Resources::VkBufferResource<>>(std::move(scratchBuffer)));

            return {vulkan.GetDevice(), std::move(accelerationStructureBuffer), std::move(accelStruct)};
        }

        void refitTLAS(
            Resources::ContextResources&                                     contextResources,
            VkCommandBuffer                                                  commandBuffer,
            Resources::VkBufferResource<VkAccelerationStructureInstanceKHR>& tlasInstancesBuffer,
            Resources::VkAccelerationStructureResource&                      outPreviousTLAS,
            Resources::Resource::ID                                          scratchBufferId)
        {
            auto& vulkan          = contextResources.GetVulkanResource();
            auto& resourceStorage = contextResources.GetResourceStorage();

            VkAccelerationStructureGeometryInstancesDataKHR geometryInstances{
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                .data  = {.deviceAddress = tlasInstancesBuffer.GetBufferDeviceAddress()}};

            VkAccelerationStructureGeometryKHR accelGeometry{
                .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
                .geometry     = {.instances = geometryInstances}};

            VkAccelerationStructureBuildRangeInfoKHR accelBuildRangeInfo{.primitiveCount = static_cast<uint32_t>(tlasInstancesBuffer.GetElementCount())};

            VkAccelerationStructureBuildGeometryInfoKHR accelBuildInfo{
                .sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                .pNext                    = nullptr,
                .type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
                .flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
                .mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR,
                .srcAccelerationStructure = outPreviousTLAS.GetAccelerationStructure(),
                .dstAccelerationStructure = outPreviousTLAS.GetAccelerationStructure(), // In place update
                .geometryCount            = 1,
                .pGeometries              = &accelGeometry,
                .ppGeometries             = nullptr,
                .scratchData              = {}, // Will set device address below
            };

            std::vector<uint32_t> maxPrimCount(1, static_cast<uint32_t>(tlasInstancesBuffer.GetElementCount()));

            VkPhysicalDeviceAccelerationStructurePropertiesKHR accelProps{};
            accelProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
            VkPhysicalDeviceProperties2 props2{};
            props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            props2.pNext = &accelProps;
            vkGetPhysicalDeviceProperties2(vulkan.GetPhysicalDevice(), &props2);

            VkAccelerationStructureBuildSizesInfoKHR buildSize{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
            vkGetAccelerationStructureBuildSizesKHR(
                vulkan.GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelBuildInfo, maxPrimCount.data(), &buildSize);

            bool needsNewScratchBuffer = false;

            auto scratchBufferOpt = resourceStorage.Get<Resources::VkBufferResource<>>(scratchBufferId);
            if (!scratchBufferOpt) {
                needsNewScratchBuffer |= true;
            } else if (buildSize.updateScratchSize > scratchBufferOpt->get().GetBufferSize()) {
                needsNewScratchBuffer |= true;
            }

            if (needsNewScratchBuffer) {
#ifdef DEBUG
                // I'm just curious if this is even possible.
                std::cout << "Reallocating TLAS scratch buffer: old size = " << scratchBufferOpt->get().GetBufferSize()
                          << " bytes, new size = " << buildSize.updateScratchSize << " bytes\n";
#endif
                Resources::VkBufferResource<> scratchBuffer{
                    vulkan.GetVmaAllocator(),
                    buildSize.updateScratchSize,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                    accelProps.minAccelerationStructureScratchOffsetAlignment};

                resourceStorage.Delete(scratchBufferId);

                resourceStorage.Insert(scratchBufferId, std::make_unique<Resources::VkBufferResource<>>(std::move(scratchBuffer)));
            }
            auto& scratchBuffer = resourceStorage.Get<Resources::VkBufferResource<>>(scratchBufferId)->get();

            accelBuildInfo.scratchData.deviceAddress = scratchBuffer.GetBufferDeviceAddress();

            VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = &accelBuildRangeInfo;

            vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelBuildInfo, &pBuildRangeInfo);
        }
    } // namespace

    RaytracedGeometryDrawingSubsystem::RaytracedGeometryDrawingSubsystem(Resources::ContextResources& contextResources) : _contextResources(contextResources)
    {
        auto& vulkanResource = _contextResources.GetVulkanResource();

        if (!(vulkanResource.GetAdditionalExtensions() & Resources::VulkanDeviceAdditionalExtensions::RAYTRACING_AVAILABLE)) {
            return;
        }

        VkDevice         device         = vulkanResource.GetDevice();
        VkPhysicalDevice physicalDevice = vulkanResource.GetPhysicalDevice();

        _descriptorPool      = createDescriptorPool(device);
        _descriptorSetLayout = createDescriptorSetLayout(device);
        _descriptorSets      = createDescriptorSets(device, _descriptorPool, _descriptorSetLayout);
        _pipelineLayout      = createPipelineLayout(device, _descriptorSetLayout);
        _pipeline            = createPipeline(device, physicalDevice, _pipelineLayout);
    };

    RaytracedGeometryDrawingSubsystem::~RaytracedGeometryDrawingSubsystem()
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

#pragma region UPDATE
    void RaytracedGeometryDrawingSubsystem::Update(
        float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::VkStagingBufferResource& stagingBuffer)
    {
        auto& resourceStorage = _contextResources.GetResourceStorage();
        auto& vmaAllocator    = _contextResources.GetVulkanResource().GetVmaAllocator();
        auto& vulkan          = _contextResources.GetVulkanResource();
        auto& registry        = scene.GetRegistry();
        auto& meshStorage     = scene.GetMeshStorage();

        if (!(vulkan.GetAdditionalExtensions() & Resources::VulkanDeviceAdditionalExtensions::RAYTRACING_AVAILABLE)) {
            return;
        }

        _tlasAccelStructToDelete     = std::nullopt;
        _tlasInstancesBufferToDelete = std::nullopt;
        _meshesInfosBufferToDelete   = std::nullopt;
        _blasToInstanceData.clear();

        auto sbtBufferOpt = resourceStorage.Get<Resources::VkBufferResource<>>(SBT_BUFFER_ID);
        if (!sbtBufferOpt) {
            auto align = [](uint32_t value, uint32_t alignment) { return (value + alignment - 1u) & ~(alignment - 1u); };

            // Query RT pipeline properties for buffer sizing and alignment
            VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
            VkPhysicalDeviceProperties2                     devProps{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &rtProps};
            vkGetPhysicalDeviceProperties2(vulkan.GetPhysicalDevice(), &devProps);

            const uint32_t handleSize      = rtProps.shaderGroupHandleSize;
            const uint32_t handleAlignment = rtProps.shaderGroupHandleAlignment;
            const uint32_t baseAlignment   = rtProps.shaderGroupBaseAlignment;

            // Each SBT entry is handleSize rounded up to handleAlignment;
            // each region is further rounded up to baseAlignment (required by the spec).
            const uint32_t entrySize  = align(handleSize, handleAlignment);
            const uint32_t regionSize = align(entrySize, baseAlignment);

            // Retrieve raw handles for all 3 groups: raygen (0), miss (1), hit (2)
            constexpr uint32_t   groupCount = 3;
            std::vector<uint8_t> handles(groupCount * handleSize);
            if (vkGetRayTracingShaderGroupHandlesKHR(vulkan.GetDevice(), _pipeline, 0, groupCount, handles.size(), handles.data()) != VK_SUCCESS) {
                throw std::runtime_error("vkGetRayTracingShaderGroupHandlesKHR failed");
            }

            // Allocate a host-visible SBT buffer: [raygen region | miss region | hit region]
            const VkDeviceSize sbtSize = 3u * static_cast<VkDeviceSize>(regionSize);

            auto sbtBuffer = Resources::VkBufferResource<>(
                vmaAllocator,
                sbtSize,
                VK_BUFFER_USAGE_2_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU);

            stagingBuffer.Copy(sbtBuffer.GetBuffer(), handles.data(), handleSize);                                  // raygen
            stagingBuffer.Copy(sbtBuffer.GetBuffer(), handles.data() + 1u * entrySize, handleSize, regionSize);     // miss
            stagingBuffer.Copy(sbtBuffer.GetBuffer(), handles.data() + 2u * entrySize, handleSize, 2 * regionSize); // hit

            const VkDeviceAddress sbtAddress = sbtBuffer.GetBufferDeviceAddress();

            _raygenShaderRegion   = {sbtAddress + 0u * regionSize, entrySize, entrySize};
            _missShaderRegion     = {sbtAddress + 1u * regionSize, entrySize, entrySize};
            _hitShaderRegion      = {sbtAddress + 2u * regionSize, entrySize, entrySize};
            _callableShaderRegion = {};

            resourceStorage.Insert<Resources::VkBufferResource<>>(SBT_BUFFER_ID, std::make_unique<Resources::VkBufferResource<>>(std::move(sbtBuffer)));
        }

        auto meshesInfosCreationBlock = [&]() {
            bool doWeNeedToRecreateMeshesInfosBuffer = false;

            auto meshesInfosBufferOpt = resourceStorage.Get<Resources::VkBufferResource<MeshInfo>>(MESHES_INFOS_BUFFER_ID);
            doWeNeedToRecreateMeshesInfosBuffer |= !meshesInfosBufferOpt.has_value();

            if (meshesInfosBufferOpt) {
                auto meshView = scene.GetRegistry().view<Components::Mesh>();

                size_t meshCount = 0;
                for (auto [_, meshComponent] : meshView.each()) {
                    auto meshOpt = meshStorage.Get<Resources::MeshResource>(meshComponent.resourceId);
                    if (!meshOpt) {
                        continue;
                    }
                    auto& mesh = meshOpt->get();

                    if (!mesh.GetID()) {
                        continue;
                    }

                    meshCount++;
                }

                if (meshesInfosBufferOpt->get().GetElementCount() != meshCount) {
                    doWeNeedToRecreateMeshesInfosBuffer |= true;
                    _meshesInfosBufferToDelete = std::move(meshesInfosBufferOpt->get());
                }
            }

            auto meshView = scene.GetRegistry().view<Components::Mesh>();
            if (doWeNeedToRecreateMeshesInfosBuffer) {
                resourceStorage.Delete(MESHES_INFOS_BUFFER_ID);

                auto meshesInfosBuffer = Resources::VkBufferResource<MeshInfo>(
                    vmaAllocator,
                    meshView.size() * sizeof(MeshInfo),
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU);

                resourceStorage.Insert<Resources::VkBufferResource<MeshInfo>>(
                    MESHES_INFOS_BUFFER_ID, std::make_unique<Resources::VkBufferResource<MeshInfo>>(std::move(meshesInfosBuffer)));
            }

            auto& meshesInfosBuffer = resourceStorage.Get<Resources::VkBufferResource<MeshInfo>>(MESHES_INFOS_BUFFER_ID)->get();

            std::vector<MeshInfo> meshesInfos;
            meshesInfos.reserve(meshView.size());

            int textureIndex = 0;
            for (auto [_, meshComponent] : meshView.each()) {
                auto meshOpt = meshStorage.Get<Resources::MeshResource>(meshComponent.resourceId);
                if (!meshOpt) {
                    continue;
                }
                auto& mesh = meshOpt->get();

                if (!mesh.GetID()) {
                    continue;
                }

                MeshInfo info{};
                info.vertexBufferAddress = mesh.GetVertexBuffer().GetBufferDeviceAddress();
                info.indexBufferAddress  = mesh.GetIndexBuffer().GetBufferDeviceAddress();
                if (mesh.GetTexture().has_value()) {
                    info.textureIndex = textureIndex;
                    textureIndex++;
                } else {
                    info.textureIndex = -1;
                }
                meshesInfos.push_back(info);
            }
            stagingBuffer.Copy(meshesInfosBuffer.GetBuffer(), meshesInfos.data(), meshesInfos.size() * sizeof(MeshInfo));

            return true;
        }();

        // BLAS creation.
        auto blasesCreationBlock = [&]() {
            auto meshView     = scene.GetRegistry().view<Components::Mesh, Components::Transform>();
            bool isEmptyScene = [&]() {
                for (auto [_, mesh, _transform] : meshView.each()) {
                    if (mesh.resourceId == Resources::MeshResource::UNINITIALIZED_ID) {
                        continue;
                    }
                    return false;
                }
                return true;
            }();
            if (isEmptyScene) {
                return false;
            }

            for (auto [meshEntity, meshComponent, transformComponent] : meshView.each()) {
                auto& meshResourceId = meshComponent.resourceId;

                auto meshOpt = meshStorage.Get<Resources::MeshResource>(meshResourceId);
                if (!meshOpt) {
                    continue;
                }
                auto& mesh = meshOpt->get();

                if (!mesh.GetID()) {
                    continue;
                }

                auto blasMeshResourceId  = std::hash<std::string_view>{}(std::format("{}/{}", MESH_BLAS_ID_PREFIX, meshResourceId));
                auto blasScratchBufferId = std::hash<std::string_view>{}(std::format("{}/{}", MESH_BLAS_SCRATCH_BUFFER_ID_PREFIX, meshResourceId));

                auto accelStructureResourceOpt = resourceStorage.Get<Resources::VkAccelerationStructureResource>(blasMeshResourceId);
                if (!accelStructureResourceOpt) {
                    auto accel = createBLASFromMesh(_contextResources, commandBuffer, mesh, blasScratchBufferId);

                    resourceStorage.Insert<Resources::VkAccelerationStructureResource>(
                        blasMeshResourceId, std::make_unique<Resources::VkAccelerationStructureResource>(std::move(accel)));
                }

                auto& blasInstances = _blasToInstanceData[blasMeshResourceId];
                if (std::find(blasInstances.begin(), blasInstances.end(), meshEntity) == blasInstances.end()) {
                    blasInstances.push_back(meshEntity);
                }
            }

            // For compiler.
            return true;
        }();

        auto tlasCreationBlock = [&]() {
            // We can't create TLAS unless we have BLASes.
            if (_blasToInstanceData.empty()) {
                return false;
            }

            std::vector<VkAccelerationStructureInstanceKHR> tlasInstances;

            // Optimization.
            size_t tlasInstancesVectorSize = 0;
            for (const auto& [_, instances] : _blasToInstanceData) {
                tlasInstancesVectorSize += instances.size();
            }
            tlasInstances.reserve(tlasInstancesVectorSize);

            // VkTransformMatrixKHR is row-major 3x4, glm::mat4 is column-major; transpose before memcpy
            auto toTransformMatrixKHR = [](const glm::mat4& m) {
                VkTransformMatrixKHR t;
                memcpy(&t, glm::value_ptr(glm::transpose(m)), sizeof(t));
                return t;
            };

            uint32_t blasIndex = 0;
            for (const auto& [blasMeshResourceId, instances] : _blasToInstanceData) {
                auto blasOpt = resourceStorage.Get<Resources::VkAccelerationStructureResource>(blasMeshResourceId);
                if (!blasOpt) {
                    continue;
                }
                auto&                                       blas = blasOpt->get();
                VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
                addressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
                addressInfo.accelerationStructure = blas.GetAccelerationStructure();

                for (const auto& instance : instances) {
                    if (registry.all_of<Components::Mesh, Components::Transform>(instance) == false) {
                        continue;
                    }
                    const auto& transform = scene.GetRegistry().get<Components::Transform>(instance);

                    VkAccelerationStructureInstanceKHR asInstance{};
                    asInstance.transform                              = toTransformMatrixKHR(transform.transform);
                    asInstance.instanceCustomIndex                    = blasIndex;
                    asInstance.accelerationStructureReference         = vkGetAccelerationStructureDeviceAddressKHR(vulkan.GetDevice(), &addressInfo);
                    asInstance.instanceShaderBindingTableRecordOffset = 0; // We will use the same hit group for all objects
                    asInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR; // No culling - double sided
                    asInstance.mask                                   = 0xFF;
                    tlasInstances.emplace_back(asInstance);
                }

                blasIndex++;
            }

            // Only in the case when number of instances changed.
            bool doWeNeedToRecreateTLAS = false;

            auto tlasInstancesOpt = resourceStorage.Get<Resources::VkBufferResource<VkAccelerationStructureInstanceKHR>>(TLAS_INSTANCES_BUFFER_ID);
            if (!tlasInstancesOpt) {
                doWeNeedToRecreateTLAS |= true;
            } else {
                auto& tlasInstancesBuffer = tlasInstancesOpt->get();
                if (tlasInstancesBuffer.GetBufferSize() < tlasInstances.size() * sizeof(VkAccelerationStructureInstanceKHR)) {
                    doWeNeedToRecreateTLAS |= true;

                    _tlasInstancesBufferToDelete = std::move(tlasInstancesBuffer);

                    auto tlasAccelStructOpt = resourceStorage.Get<Resources::VkAccelerationStructureResource>(TLAS_ACCEL_STRUCT_ID);
                    if (tlasAccelStructOpt) {
                        auto& tlasAccelStruct = tlasAccelStructOpt->get();

                        _tlasAccelStructToDelete = std::move(tlasAccelStruct);
                    }

                    resourceStorage.Delete(TLAS_INSTANCES_BUFFER_ID);
                    resourceStorage.Delete(TLAS_ACCEL_STRUCT_ID);
                }
            }

            if (doWeNeedToRecreateTLAS) {
                Resources::VkBufferResource<VkAccelerationStructureInstanceKHR> tlasInstancesBuffer{
                    vmaAllocator,
                    sizeof(VkAccelerationStructureInstanceKHR) * tlasInstances.size(),
                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU};

                // TLAS acceleration structure needs it immediately.
                stagingBuffer.CopyImmediately(
                    commandBuffer,
                    tlasInstancesBuffer.GetAllocation(),
                    tlasInstances.data(),
                    sizeof(VkAccelerationStructureInstanceKHR) * tlasInstances.size());

                auto tlasAccelStruct = createTLAS(_contextResources, commandBuffer, tlasInstancesBuffer, tlasInstances, TLAS_SCRATCH_BUFFER_ID);

                resourceStorage.Insert<Resources::VkAccelerationStructureResource>(
                    TLAS_ACCEL_STRUCT_ID, std::make_unique<Resources::VkAccelerationStructureResource>(std::move(tlasAccelStruct)));

                resourceStorage.Insert<Resources::VkBufferResource<VkAccelerationStructureInstanceKHR>>(
                    TLAS_INSTANCES_BUFFER_ID,
                    std::make_unique<Resources::VkBufferResource<VkAccelerationStructureInstanceKHR>>(std::move(tlasInstancesBuffer)));
            } else {
                auto& tlasInstancesBuffer =
                    resourceStorage.Get<Resources::VkBufferResource<VkAccelerationStructureInstanceKHR>>(TLAS_INSTANCES_BUFFER_ID)->get();

                stagingBuffer.CopyImmediately(
                    commandBuffer,
                    tlasInstancesBuffer.GetAllocation(),
                    tlasInstances.data(),
                    sizeof(VkAccelerationStructureInstanceKHR) * tlasInstances.size());

                auto& previousTLAS = resourceStorage.Get<Resources::VkAccelerationStructureResource>(TLAS_ACCEL_STRUCT_ID)->get();

                refitTLAS(_contextResources, commandBuffer, tlasInstancesBuffer, previousTLAS, TLAS_SCRATCH_BUFFER_ID);
            }

            // For compiler.
            return true;
        }();
    };

    void RaytracedGeometryDrawingSubsystem::Render(
        float deltaTime, VkCommandBuffer commandBuffer, Resources::Scene& scene, Resources::RenderTargetResource& renderTarget)
    {
        auto& resourceStorage = _contextResources.GetResourceStorage();
        auto& vulkanResource  = _contextResources.GetVulkanResource();
        auto  currentFrame    = vulkanResource.GetCurrentFrameOffset();

        auto commonUniformBufferOpt =
            resourceStorage.Get<Resources::VkBufferResource<Resources::CommonResource>>(Resources::CommonResource::UNIFORM_BUFFER_ID, currentFrame);
        if (!commonUniformBufferOpt) {
            return;
        }
        auto& commonUniformBuffer  = commonUniformBufferOpt->get();
        auto  meshesInfosBufferOpt = resourceStorage.Get<Resources::VkBufferResource<MeshInfo>>(MESHES_INFOS_BUFFER_ID);
        if (!meshesInfosBufferOpt) {
            return;
        }
        auto& meshesInfos = meshesInfosBufferOpt->get();

        auto tlasOpt = resourceStorage.Get<Resources::VkAccelerationStructureResource>(TLAS_ACCEL_STRUCT_ID);
        if (!tlasOpt) {
            return;
        }
        VkAccelerationStructureKHR tlas = tlasOpt->get().GetAccelerationStructure();

        // Transition the color image to VK_IMAGE_LAYOUT_GENERAL for storage-image writes
        VkImageMemoryBarrier2 toGeneral{};
        toGeneral.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        toGeneral.srcStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        toGeneral.srcAccessMask    = VK_ACCESS_2_NONE;
        toGeneral.dstStageMask     = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
        toGeneral.dstAccessMask    = VK_ACCESS_2_SHADER_WRITE_BIT;
        toGeneral.oldLayout        = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        toGeneral.newLayout        = VK_IMAGE_LAYOUT_GENERAL;
        toGeneral.image            = renderTarget.GetColorImage();
        toGeneral.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        VkDependencyInfo depInfoToGeneral{};
        depInfoToGeneral.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfoToGeneral.imageMemoryBarrierCount = 1;
        depInfoToGeneral.pImageMemoryBarriers    = &toGeneral;
        vkCmdPipelineBarrier2(commandBuffer, &depInfoToGeneral);

        auto& registry          = scene.GetRegistry();
        auto& meshStorage       = scene.GetMeshStorage();
        auto  meshTransformView = registry.view<Components::Mesh, Components::Transform>();

        std::vector<VkDescriptorImageInfo> textureImageInfos;
        textureImageInfos.reserve(meshTransformView.size_hint());
        for (const auto meshEntity : meshTransformView) {
            const auto& meshResourceId = meshTransformView.get<Components::Mesh>(meshEntity).resourceId;

            auto meshOpt = meshStorage.Get<Resources::MeshResource>(meshResourceId);
            if (!meshOpt) {
                continue;
            }
            auto& mesh = meshOpt->get();

            if (mesh.GetTexture().has_value()) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView   = mesh.GetTexture()->GetImageView();
                imageInfo.sampler     = mesh.GetTexture()->GetSampler();
                textureImageInfos.push_back(imageInfo);
            }
        }

        updateDescriptorSet(
            vulkanResource.GetDevice(),
            _descriptorSets[currentFrame],
            commonUniformBuffer.GetBuffer(),
            textureImageInfos,
            meshesInfos.GetBuffer(),
            tlas,
            renderTarget.GetColorImageView());

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _pipelineLayout, 0, 1, &_descriptorSets[currentFrame], 0, nullptr);

        const auto extent = vulkanResource.GetSwapchainExtent();
        vkCmdTraceRaysKHR(commandBuffer, &_raygenShaderRegion, &_missShaderRegion, &_hitShaderRegion, &_callableShaderRegion, extent.width, extent.height, 1);

        // Transition the color image back to COLOR_ATTACHMENT_OPTIMAL
        VkImageMemoryBarrier2 toAttachment{};
        toAttachment.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        toAttachment.srcStageMask     = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
        toAttachment.srcAccessMask    = VK_ACCESS_2_SHADER_WRITE_BIT;
        toAttachment.dstStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        toAttachment.dstAccessMask    = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        toAttachment.oldLayout        = VK_IMAGE_LAYOUT_GENERAL;
        toAttachment.newLayout        = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        toAttachment.image            = renderTarget.GetColorImage();
        toAttachment.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        VkDependencyInfo depInfoToAttachment{};
        depInfoToAttachment.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfoToAttachment.imageMemoryBarrierCount = 1;
        depInfoToAttachment.pImageMemoryBarriers    = &toAttachment;
        vkCmdPipelineBarrier2(commandBuffer, &depInfoToAttachment);
    }
} // namespace Prism::Systems::Subsystems::MeshDrawingSystem