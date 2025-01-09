#pragma once

#include "VulkanContext.h"
#include "DescriptorSetAllocator.h"
#include "Image.h"
#include "Buffer.h"
#include "Pass.h"

namespace rendering {

    class ComputePass : public Pass {
    public:
        ComputePass(std::string shaderPath, std::string entry, const std::vector<int> &specializationConstants);

        ComputePass(ComputePass &other) = delete;

        ComputePass(ComputePass &&other) noexcept;

        ComputePass &operator=(ComputePass &other) = delete;

        ComputePass& operator=(ComputePass&& other) noexcept;

        void build(const VulkanContext &context,
                   DescriptorSetAllocator &descriptorSetAllocator,
                   const vk::DescriptorSetLayout &uniformDescriptorSetLayout) override;

        void dispatch(
                vk::CommandBuffer commandBuffer,
                const vk::DescriptorSet &uniformDescriptorSet,
                uint32_t width,
                uint32_t height,
                uint32_t depth
        ) override;


    private:
        std::string shaderPath;
        std::string entry;
        std::vector<int> specializationConstants;

    };

} // rendering