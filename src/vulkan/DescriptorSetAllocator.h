#pragma once

#include "VulkanContext.h"

namespace rendering {

    struct PoolSizeInfo {
        vk::DescriptorType type;
        float ratio;
    };

    class DescriptorSetAllocator {
    public:
        explicit DescriptorSetAllocator(VulkanContext &poolSizeInfo, uint32_t maxSets,
                                        const std::vector<PoolSizeInfo> &poolSizeInfos);

        vk::UniqueDescriptorSet allocate(const VulkanContext &context, const vk::DescriptorSetLayout &layout);
        void reset(VulkanContext &context);
    private:
        vk::UniqueDescriptorPool pool;

    };
}