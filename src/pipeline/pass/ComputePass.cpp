#include "ComputePass.h"

#include <utility>

#include "Image.h"
#include "util.h"

namespace rendering {

    ComputePass::ComputePass(std::string shaderPath, std::string entry, const std::vector<int> &specializationConstants)
            : shaderPath(std::move(shaderPath)),
              entry(std::move(entry)),
              specializationConstants(specializationConstants) {
    }

    void ComputePass::build(
            const VulkanContext &context,
            DescriptorSetAllocator &descriptorSetAllocator,
            const vk::DescriptorSetLayout &uniformDescriptorSetLayout
    ) {
        Pass::build(context, descriptorSetAllocator, uniformDescriptorSetLayout);

        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {uniformDescriptorSetLayout, *descriptorSetLayout};
        pipelineLayout = context.device->createPipelineLayoutUnique({{}, descriptorSetLayouts});

        std::vector<vk::SpecializationMapEntry> specEntries(specializationConstants.size());
        for (size_t i = 0; i < specEntries.size(); i++) {
            specEntries[i].offset = sizeof(int) * i;
            specEntries[i].size = sizeof(int);
            specEntries[i].constantID = i;
        }

        vk::SpecializationInfo specializationInfo{
                static_cast<uint32_t>(specEntries.size()),
                specEntries.data(),
                sizeof(int) * specEntries.size(),
                specializationConstants.data()
        };

        const auto shaderModule = createShader(context, shaderPath);

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{
                {},
                vk::ShaderStageFlagBits::eCompute,
                *shaderModule,
                entry.c_str(),
                &specializationInfo,
        };

        vk::ComputePipelineCreateInfo pipelineCreateInfo{
                {},
                shaderStageCreateInfo,
                *pipelineLayout,
        };
        pipeline = context.device->createComputePipelineUnique(nullptr, pipelineCreateInfo).value;
    }

    void ComputePass::dispatch(
            vk::CommandBuffer commandBuffer,
            const vk::DescriptorSet &uniformDescriptorSet,
            uint32_t width,
            uint32_t height,
            uint32_t depth
    ) {
        Pass::dispatch(commandBuffer, uniformDescriptorSet, width, height, depth);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
        commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eCompute, *pipelineLayout, 0, {uniformDescriptorSet, *descriptorSet}, nullptr
        );
        commandBuffer.dispatch(width, height, depth);
    }

    ComputePass::ComputePass(ComputePass &&other) noexcept
            : Pass(std::move(other)),
              shaderPath(std::move(other.shaderPath)),
              entry(std::move(other.entry)),
              specializationConstants(std::move(other.specializationConstants)) {
    }

    ComputePass &ComputePass::operator=(ComputePass &&other) noexcept {
        Pass::operator=(std::move(other));
        shaderPath = std::move(other.shaderPath);
        entry = std::move(other.entry);
        specializationConstants = std::move(other.specializationConstants);
        return *this;
    }

}  // namespace rendering
