#include "Pass.h"

namespace rendering {

    void Pass::addInput(uint32_t binding, vk::DescriptorType type, uint32_t descriptorCount) {
        bindings.emplace_back(binding, type, descriptorCount, vk::ShaderStageFlagBits::eAll);
    }

    void Pass::setInputImage(const VulkanContext &context, uint32_t binding, const Image &resource) {
        vk::DescriptorImageInfo imageInfo{
                {},
                *resource.view,
                vk::ImageLayout::eGeneral,
        };
        vk::WriteDescriptorSet writeDescriptorSet{
                *descriptorSet,
                binding,
                0,
                vk::DescriptorType::eStorageImage,
                imageInfo,
        };
        context.device->updateDescriptorSets(writeDescriptorSet, nullptr);
        vk::ImageMemoryBarrier2 barrier{
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eGeneral,
                {},
                {},
                *resource.image,
                vk::ImageSubresourceRange{
                        vk::ImageAspectFlagBits::eColor,
                        0, vk::RemainingMipLevels,
                        0, vk::RemainingArrayLayers,
                },
        };
        imageBarriers.push_back(barrier);
    }

    void Pass::setInputImageArray(const VulkanContext &context, uint32_t binding, const std::vector<Image> &resources) {
        std::vector<vk::DescriptorImageInfo> imageInfos;
        for (const auto &resource: resources) {
            imageInfos.push_back(
                    vk::DescriptorImageInfo{
                            {},
                            *resource.view,
                            vk::ImageLayout::eGeneral
                    }
            );

            vk::ImageMemoryBarrier2 barrier{
                    vk::PipelineStageFlagBits2::eAllCommands,
                    vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
                    vk::PipelineStageFlagBits2::eAllCommands,
                    vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                    vk::ImageLayout::eGeneral,
                    vk::ImageLayout::eGeneral,
                    {},
                    {},
                    *resource.image,
                    vk::ImageSubresourceRange{
                            vk::ImageAspectFlagBits::eColor,
                            0, vk::RemainingMipLevels,
                            0, vk::RemainingArrayLayers,
                    },
            };
            imageBarriers.push_back(barrier);
        }
        vk::WriteDescriptorSet writeDescriptorSet{
                *descriptorSet,
                binding,
                0,
                vk::DescriptorType::eStorageImage,
                imageInfos,
        };
        context.device->updateDescriptorSets(writeDescriptorSet, nullptr);
    }

    void Pass::setInputSampler(
            const VulkanContext &context,
            uint32_t binding,
            const vk::Sampler &sampler,
            const Image &image
    ) {
        vk::DescriptorImageInfo imageInfo{
                sampler,
                *image.view,
                vk::ImageLayout::eGeneral,
        };
        vk::WriteDescriptorSet writeDescriptorSet{
                *descriptorSet,
                binding,
                0,
                vk::DescriptorType::eCombinedImageSampler,
                imageInfo,
        };
        context.device->updateDescriptorSets(writeDescriptorSet, nullptr);

        vk::ImageMemoryBarrier2 barrier{
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eGeneral,
                {},
                {},
                *image.image,
                vk::ImageSubresourceRange{
                        vk::ImageAspectFlagBits::eColor,
                        0, vk::RemainingMipLevels,
                        0, vk::RemainingArrayLayers,
                },
        };
        imageBarriers.push_back(barrier);
    }

    void Pass::setInputBuffer(const VulkanContext &context, uint32_t binding, const Buffer &resource, size_t size) {
        vk::DescriptorBufferInfo descriptorBufferInfo{
                *resource.buffer,
                {},
                size,
        };
        vk::WriteDescriptorSet writeDescriptorSet = {
                *descriptorSet,
                binding,
                1,
                vk::DescriptorType::eStorageImage,
                {},
                descriptorBufferInfo,
        };
        context.device->updateDescriptorSets(writeDescriptorSet, nullptr);
        vk::BufferMemoryBarrier2 barrier{
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                {},
                {},
                *resource.buffer,
        };
        bufferBarriers.push_back(barrier);
    }

    Pass::Pass(Pass &&other) noexcept
            : descriptorSetLayout(std::move(other.descriptorSetLayout)),
              descriptorSet(std::move(other.descriptorSet)),
              bufferBarriers(std::move(other.bufferBarriers)),
              imageBarriers(std::move(other.imageBarriers)),
              bindings(std::move(other.bindings)),
              pipelineLayout(std::move(other.pipelineLayout)),
              pipeline(std::move(other.pipeline)) {
    }

    Pass &Pass::operator=(Pass &&other) noexcept {
        descriptorSetLayout = std::move(other.descriptorSetLayout);
        descriptorSet = std::move(other.descriptorSet);
        bufferBarriers = std::move(other.bufferBarriers);
        imageBarriers = std::move(other.imageBarriers);
        bindings = std::move(other.bindings);
        pipelineLayout = std::move(other.pipelineLayout);
        pipeline = std::move(other.pipeline);
        return *this;
    }

    void Pass::build(
            const VulkanContext &context,
            DescriptorSetAllocator &descriptorSetAllocator,
            const vk::DescriptorSetLayout &uniformDescriptorSetLayout
    ) {
        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
                {},
                bindings,
        };

        descriptorSetLayout = context.device->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);
        descriptorSet = descriptorSetAllocator.allocate(context, *descriptorSetLayout);

        imageBarriers.clear();
        bufferBarriers.clear();
    }

    void Pass::dispatch(
            vk::CommandBuffer commandBuffer,
            const vk::DescriptorSet &uniformDescriptorSet,
            uint32_t width,
            uint32_t height,
            uint32_t depth
    ) {
        vk::DependencyInfo dependencyInfo{};
        dependencyInfo.setImageMemoryBarriers(imageBarriers).setBufferMemoryBarriers(bufferBarriers);
        commandBuffer.pipelineBarrier2(dependencyInfo);
    }

}  // namespace rendering
