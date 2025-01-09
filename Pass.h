#ifndef DIPTERV_RT_PASS_H
#define DIPTERV_RT_PASS_H

#include "VulkanContext.h"

#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "DescriptorSetAllocator.h"
#include "Image.h"
#include "Buffer.h"

namespace rendering {

    class Pass {
    public:
        virtual void build(
                const VulkanContext &context,
                DescriptorSetAllocator &descriptorSetAllocator,
                const vk::DescriptorSetLayout &uniformDescriptorSetLayout
        );

        virtual void dispatch(
                vk::CommandBuffer commandBuffer,
                const vk::DescriptorSet &uniformDescriptorSet,
                uint32_t width,
                uint32_t height,
                uint32_t depth
        );

        void addInput(uint32_t binding, vk::DescriptorType type, uint32_t descriptorCount);

        void setInputImage(const VulkanContext &context, uint32_t binding, const rendering::Image &resource);

        void setInputImageArray(const VulkanContext &context, uint32_t binding, const std::vector<Image> &resources);

        void
        setInputBuffer(const VulkanContext &context, uint32_t binding, const rendering::Buffer &resource, size_t size);

        void
        setInputSampler(const VulkanContext &context, uint32_t binding, const vk::Sampler &sampler, const Image &image);

        Pass() = default;

        Pass(Pass &other) = delete;

        Pass(Pass &&other) noexcept;

        Pass &operator=(Pass &&other) noexcept;

        virtual ~Pass() = default;

    protected:
        vk::UniqueDescriptorSetLayout descriptorSetLayout;
        vk::UniqueDescriptorSet descriptorSet;
        std::vector<vk::BufferMemoryBarrier2> bufferBarriers;
        std::vector<vk::ImageMemoryBarrier2> imageBarriers;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline pipeline;
    };

} // rendering

#endif //DIPTERV_RT_PASS_H
