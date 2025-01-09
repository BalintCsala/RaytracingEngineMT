#include "RaytracePass.h"

#include <utility>

#include "util.h"

namespace rendering {
    RaytracePass::RaytracePass(
            std::shared_ptr<Scene> scene,
            std::string rayGenPath,
            const std::vector<std::string> &rayMissPaths,
            const std::vector<std::string> &rayClosestHitPaths,
            const std::vector<std::string> &rayAnyHitPaths
    )
            : scene(std::move(scene)),
              rayGenPath(std::move(rayGenPath)),
              rayMissPaths(rayMissPaths),
              rayClosestHitPaths(rayClosestHitPaths),
              rayAnyHitPaths(rayAnyHitPaths) {
    }

    void RaytracePass::build(
            const VulkanContext &context,
            DescriptorSetAllocator &descriptorSetAllocator,
            const vk::DescriptorSetLayout &uniformDescriptorSetLayout
    ) {
        Pass::build(context, descriptorSetAllocator, uniformDescriptorSetLayout);

        std::vector<vk::UniqueShaderModule> shaders;
        std::vector<vk::PipelineShaderStageCreateInfo> stageCreateInfos;
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groupCreateInfos;

        shaders.push_back(createShader(context, rayGenPath));
        stageCreateInfos.emplace_back(
                vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eRaygenKHR, *shaders[0], "main"
        );
        groupCreateInfos.emplace_back(
                vk::RayTracingShaderGroupTypeKHR::eGeneral,
                0,
                VK_SHADER_UNUSED_KHR,
                VK_SHADER_UNUSED_KHR,
                VK_SHADER_UNUSED_KHR);
        for (const auto &path: rayMissPaths) {
            shaders.push_back(createShader(context, path));
            stageCreateInfos.push_back(
                    vk::PipelineShaderStageCreateInfo{
                            {},
                            vk::ShaderStageFlagBits::eMissKHR,
                            *shaders[shaders.size() - 1],
                            "main"
                    }
            );
            groupCreateInfos.emplace_back(
                    vk::RayTracingShaderGroupTypeKHR::eGeneral,
                    static_cast<uint32_t>(shaders.size() - 1),
                    VK_SHADER_UNUSED_KHR,
                    VK_SHADER_UNUSED_KHR,
                    VK_SHADER_UNUSED_KHR
            );
        }
        for (size_t i = 0; i < rayClosestHitPaths.size(); i++) {
            const auto closestHitPath = rayClosestHitPaths[i];
            const auto anyHitPath = rayAnyHitPaths[i];
            shaders.push_back(createShader(context, closestHitPath));
            shaders.push_back(createShader(context, anyHitPath));

            stageCreateInfos.push_back(
                    vk::PipelineShaderStageCreateInfo{{},
                                                      vk::ShaderStageFlagBits::eClosestHitKHR,
                                                      *shaders[shaders.size() - 2],
                                                      "main"
                    }
            );
            stageCreateInfos.push_back(
                    vk::PipelineShaderStageCreateInfo{{},
                                                      vk::ShaderStageFlagBits::eAnyHitKHR,
                                                      *shaders[shaders.size() - 1],
                                                      "main"
                    }
            );
            groupCreateInfos.emplace_back(
                    vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                    VK_SHADER_UNUSED_KHR,
                    static_cast<uint32_t>(shaders.size() - 2),
                    static_cast<uint32_t>(shaders.size() - 1),
                    VK_SHADER_UNUSED_KHR
            );
        }

        std::vector<vk::DescriptorSetLayoutBinding> sceneBindings = {
                {0, vk::DescriptorType::eAccelerationStructureKHR, 1,                                                        vk::ShaderStageFlagBits::eAll},
                {1, vk::DescriptorType::eStorageBuffer,            1,                                                        vk::ShaderStageFlagBits::eAll},
                {2, vk::DescriptorType::eCombinedImageSampler,     static_cast<uint32_t>(scene->textureCache.images.size()), vk::ShaderStageFlagBits::eAll},
                {3, vk::DescriptorType::eStorageBuffer,            1,                                                        vk::ShaderStageFlagBits::eAll},
        };

        sceneDescriptorSetLayout = context.device->createDescriptorSetLayoutUnique(
                {
                        {},
                        sceneBindings
                }
        );

        sceneDescriptorSet = descriptorSetAllocator.allocate(context, *sceneDescriptorSetLayout);

        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {
                uniformDescriptorSetLayout, *descriptorSetLayout, *sceneDescriptorSetLayout
        };
        pipelineLayout = context.device->createPipelineLayoutUnique(
                {
                        {},
                        descriptorSetLayouts,
                        nullptr
                }
        );

        std::vector<vk::WriteDescriptorSet> writes(sceneBindings.size());
        for (size_t i = 0; i < sceneBindings.size(); ++i) {
            writes[i]
                    .setDstSet(*sceneDescriptorSet)
                    .setDescriptorType(sceneBindings[i].descriptorType)
                    .setDescriptorCount(
                            sceneBindings[i].descriptorCount
                    )
                    .setDstBinding(sceneBindings[i].binding);
        }

        vk::DescriptorBufferInfo descriptorBufferInfo{
                *scene->objDescriptorBuffer->buffer,
                {},
                sizeof(rendering::ObjDesc) * scene->objects.size()
        };
        vk::DescriptorBufferInfo emissiveIdsInfo{
                *scene->emissiveObjectIdsBuffer->buffer,
                {},
                sizeof(uint32_t) * scene->emissiveObjectIds.size()
        };
        std::vector<vk::DescriptorImageInfo> imageInfos;
        for (const auto &image: scene->textureCache.images) {
            imageInfos.emplace_back(*scene->sampler, *image->view, vk::ImageLayout::eGeneral);
        }

        writes[0].setPNext(&scene->accelerationStructure->accelInfo);
        writes[1].setBufferInfo(descriptorBufferInfo);
        writes[2].setImageInfo(imageInfos);
        writes[3].setBufferInfo(emissiveIdsInfo);

        context.device->updateDescriptorSets(writes, nullptr);

        vk::RayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{
                {},
                stageCreateInfos,
                groupCreateInfos,
                1,
                {},
                {},
                {},
                *pipelineLayout,
        };

        auto result = context.device->createRayTracingPipelinesKHRUnique({}, {}, rayTracingPipelineCreateInfo);
        pipeline = std::move(result.value[0]);

        const auto missCount = rayMissPaths.size();
        const auto hitCount = rayClosestHitPaths.size();

        auto properties = context.physicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
        auto rtProperties = properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

        const auto handleCount = static_cast<uint32_t>(groupCreateInfos.size());
        const auto handleSize = rtProperties.shaderGroupHandleSize;
        const auto handleSizeAligned = alignUp(handleSize, rtProperties.shaderGroupHandleAlignment);

        rayGenRegion.stride = alignUp(handleSizeAligned, rtProperties.shaderGroupBaseAlignment);
        rayGenRegion.size = rayGenRegion.stride;

        rayMissRegion.stride = handleSizeAligned;
        rayMissRegion.size = alignUp(missCount * handleSizeAligned, rtProperties.shaderGroupBaseAlignment);

        rayHitRegion.stride = handleSizeAligned;
        rayHitRegion.size = alignUp(hitCount * handleSizeAligned, rtProperties.shaderGroupBaseAlignment);

        const auto dataSize = handleCount * handleSize;
        std::vector<uint8_t> handles(dataSize);
        if (context.device->getRayTracingShaderGroupHandlesKHR(
                *pipeline, 0, handleCount, dataSize, handles.data()) != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to get ray tracing group handles");
        }

        const auto sbtSize = rayGenRegion.size + rayMissRegion.size + rayHitRegion.size;
        std::vector<uint8_t> sbtData(sbtSize);
        uint32_t offs = 0;
        uint8_t *region = sbtData.data();
        memcpy(region, handles.data() + (offs++) * handleSize, handleSize);

        region = sbtData.data() + rayGenRegion.size;
        for (size_t i = 0; i < rayMissPaths.size(); i++) {
            memcpy(region, handles.data() + (offs++) * handleSize, handleSize);
            region += rayMissRegion.stride;
        }

        region = sbtData.data() + rayGenRegion.size + rayMissRegion.size;
        for (size_t i = 0; i < rayClosestHitPaths.size(); i++) {
            memcpy(region, handles.data() + (offs++) * handleSize, handleSize);
            region += rayHitRegion.stride;
        }

        shaderBindingTableBuffer = std::make_unique<Buffer>(
                context,
                sbtSize,
                vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                sbtData.data()
        );

        rayGenRegion.deviceAddress = shaderBindingTableBuffer->deviceAddress();
        rayMissRegion.deviceAddress = rayGenRegion.deviceAddress + rayGenRegion.size;
        rayHitRegion.deviceAddress = rayMissRegion.deviceAddress + rayMissRegion.size;
    }

    void RaytracePass::dispatch(
            vk::CommandBuffer commandBuffer,
            const vk::DescriptorSet &uniformDescriptorSet,
            uint32_t width,
            uint32_t height,
            uint32_t depth
    ) {
        Pass::dispatch(commandBuffer, uniformDescriptorSet, width, height, depth);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *pipeline);
        commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eRayTracingKHR,
                *pipelineLayout,
                0,
                {uniformDescriptorSet, *descriptorSet, *sceneDescriptorSet},
                {}
        );
        commandBuffer.traceRaysKHR(
                rayGenRegion,
                rayMissRegion,
                rayHitRegion,
                {},
                width,
                height,
                depth
        );
    }

}  // namespace rendering
