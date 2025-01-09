#ifndef DIPTERV_RT_PROCESSINGPIPELINE_H
#define DIPTERV_RT_PROCESSINGPIPELINE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ComputePass.h"
#include "Image.h"
#include "Scene.h"

namespace rendering {
    class ProcessingPipeline {
    public:
        std::unique_ptr<Image> outputImage;

        explicit ProcessingPipeline(const std::string &path, const std::shared_ptr<Scene> &scene);

        void build(VulkanContext &context, DescriptorSetAllocator &allocator, vk::Extent2D screenSize);

        void dispatch(vk::CommandBuffer commandBuffer);

        template<typename T>
        inline void
        updateUniforms(const VulkanContext &context, const vk::Buffer &uniforms, const vk::Buffer &prevUniforms) {
            vk::DescriptorBufferInfo uniformsBufferInfo{
                    uniforms,
                    {},
                    sizeof(T),
            };
            vk::DescriptorBufferInfo prevUniformsBufferInfo{
                    prevUniforms,
                    {},
                    sizeof(T),
            };
            std::vector<vk::WriteDescriptorSet> writes = {
                    {
                            *uniformDescriptorSet,
                            0,
                            0,
                            vk::DescriptorType::eUniformBuffer,
                            {},
                            uniformsBufferInfo,
                    },
                    {
                            *uniformDescriptorSet,
                            1,
                            0,
                            vk::DescriptorType::eUniformBuffer,
                            {},
                            prevUniformsBufferInfo,
                    }
            };
            context.device->updateDescriptorSets(writes, {});
        }

    private:
        struct ImageDesc {
            std::string name;
            std::string width;
            std::string height;
            vk::Format format;
            int32_t count;
        };
        struct SamplerDesc {
            std::string name;
            std::string image;
            bool bilinear;
            bool clamp;
        };
        struct PassDesc {
            std::vector<std::string> bindings;
            std::string width;
            std::string height;
            std::string depth;
        };

        std::vector<ImageDesc> imageDescriptions;
        std::vector<SamplerDesc> samplerDescriptions;
        std::vector<PassDesc> passDescriptions;

        std::unordered_map<std::string, std::unique_ptr<Image>> images;
        std::unordered_map<std::string, std::vector<Image>> imageArrays;
        std::unordered_map<std::string, vk::UniqueSampler> samplers;
        std::vector<std::unique_ptr<Pass>> passes;
        std::vector<vk::Extent3D> dispatchSizes;
        vk::UniqueDescriptorSetLayout uniformDescriptorSetLayout;
        vk::UniqueDescriptorSet uniformDescriptorSet;
    };

} // rendering

#endif //DIPTERV_RT_PROCESSINGPIPELINE_H
