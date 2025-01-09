#ifndef DIPTERV_RT_RAYTRACEPASS_H
#define DIPTERV_RT_RAYTRACEPASS_H

#include <memory>

#include "Pass.h"
#include "Scene.h"

namespace rendering {

class RaytracePass : public Pass {
public:
    RaytracePass(std::shared_ptr<Scene> scene,
                 std::string rayGenPath,
                 const std::vector<std::string> &rayMissPaths,
                 const std::vector<std::string> &rayClosestHitPaths,
                 const std::vector<std::string> &rayAnyHitPaths);

    RaytracePass(const RaytracePass &) = delete;

    RaytracePass &operator=(const RaytracePass &) = delete;

    RaytracePass(RaytracePass &&other) = delete;

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
    std::shared_ptr<Scene> scene;

    std::string rayGenPath;
    std::vector<std::string> rayMissPaths;
    std::vector<std::string> rayClosestHitPaths;
    std::vector<std::string> rayAnyHitPaths;

    vk::StridedDeviceAddressRegionKHR rayGenRegion;
    vk::StridedDeviceAddressRegionKHR rayMissRegion;
    vk::StridedDeviceAddressRegionKHR rayHitRegion;
    std::unique_ptr<Buffer> shaderBindingTableBuffer;

    vk::UniqueDescriptorSetLayout sceneDescriptorSetLayout;
    vk::UniqueDescriptorSet sceneDescriptorSet;
};

}  // namespace rendering

#endif  // DIPTERV_RT_RAYTRACEPASS_H
