#include "DescriptorSetAllocator.h"

#include <numeric>

namespace rendering {

DescriptorSetAllocator::DescriptorSetAllocator(VulkanContext &context,
                                               uint32_t maxSets,
                                               const std::vector<PoolSizeInfo> &poolSizeInfos) {
    std::vector<vk::DescriptorPoolSize> poolSizes;
    for (const auto &info : poolSizeInfos) {
        poolSizes.emplace_back(info.type, static_cast<uint32_t>(static_cast<float>(maxSets) * info.ratio));
    }

    vk::DescriptorPoolCreateInfo createInfo{
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        maxSets,
        poolSizes,
    };
    pool = context.device->createDescriptorPoolUnique(createInfo);
}

vk::UniqueDescriptorSet DescriptorSetAllocator::allocate(const VulkanContext &context,
                                                         const vk::DescriptorSetLayout &layout) {
    vk::DescriptorSetAllocateInfo allocateInfo{
        *pool,
        layout,
    };
    return std::move(context.device->allocateDescriptorSetsUnique(allocateInfo).front());
}

void DescriptorSetAllocator::reset(VulkanContext &context) {
    context.device->resetDescriptorPool(*pool);
}
}  // namespace rendering
