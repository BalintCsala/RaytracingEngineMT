#include "ProcessingPipeline.h"

#include <fstream>

#include "RaytracePass.h"
#include "expression_parsing.h"
#include "json.hpp"

vk::Format getVulkanFormat(const std::string &format) {
    if (format == "rgba32f") {
        return vk::Format::eR32G32B32A32Sfloat;
    } else if (format == "rgb32f") {
        return vk::Format::eR32G32B32Sfloat;
    } else if (format == "rgba8") {
        return vk::Format::eR8G8B8A8Unorm;
    } else if (format == "r32f") {
        return vk::Format::eR32Sfloat;
    }
    throw std::runtime_error("Unhandled format: " + format);
}

namespace rendering {
    ProcessingPipeline::ProcessingPipeline(const std::string &path, const std::shared_ptr<Scene> &scene) {
        std::ifstream input(path);
        auto data = nlohmann::json::parse(input);
        std::unordered_map<std::string, vk::DescriptorType> bindingTypes;
        std::unordered_map<std::string, int32_t> descriptorCounts;

        auto imageData = data["images"].template get<std::vector<nlohmann::json>>();
        for (const auto &image: imageData) {
            auto name = image["name"].template get<std::string>();
            auto width = image["width"].template get<std::string>();
            auto height = image["height"].template get<std::string>();
            auto format = getVulkanFormat(image["format"].template get<std::string>());
            auto count = 1;
            if (image.contains("count")) {
                count = image["count"].template get<int32_t>();
            }
            imageDescriptions.emplace_back(name, width, height, format, count);
            bindingTypes[name] = vk::DescriptorType::eStorageImage;
            descriptorCounts[name] = count;
        }

        auto samplerData = data["samplers"].template get<std::vector<nlohmann::json>>();
        for (const auto &sampler: samplerData) {
            auto name = sampler["name"].template get<std::string>();
            auto image = sampler["image"].template get<std::string>();
            auto bilinear = sampler["bilinear"].template get<bool>();
            auto clamp = sampler["clamp"].template get<bool>();
            samplerDescriptions.push_back(
                    SamplerDesc{
                            name,
                            image,
                            bilinear,
                            clamp
                    }
            );
            bindingTypes[name] = vk::DescriptorType::eCombinedImageSampler;
        }

        auto passDatas = data["passes"].template get<std::vector<nlohmann::json>>();
        for (const auto &passData: passDatas) {
            auto type = passData["type"].template get<std::string>();
            auto width = passData["width"].template get<std::string>();
            auto height = passData["height"].template get<std::string>();
            auto depth = passData["depth"].template get<std::string>();
            auto bindings = passData["bindings"].template get<std::vector<std::string>>();
            passDescriptions.push_back(
                    {
                            .bindings = bindings,
                            .width = width,
                            .height = height,
                            .depth = depth,
                    }
            );
            std::unique_ptr<Pass> pass;
            if (type == "compute") {
                auto shader = passData["shader"].template get<std::string>();
                auto entry = passData["entry"].template get<std::string>();
                auto specializations = passData["specializations"].template get<std::vector<int32_t>>();
                pass = std::make_unique<ComputePass>(shader, entry, specializations);
            } else if (type == "raytrace") {
                auto rgen = passData["rgen"].template get<std::string>();
                auto rchit = passData["rchit"].template get<std::vector<std::string>>();
                auto rahit = passData["rahit"].template get<std::vector<std::string>>();
                auto rmiss = passData["rmiss"].template get<std::vector<std::string>>();
                pass = std::make_unique<RaytracePass>(scene, rgen, rmiss, rchit, rahit);
            }
            for (size_t i = 0; i < bindings.size(); i++) {
                auto binding = bindings[i];
                vk::DescriptorType descriptorType;
                auto count = 1;
                if (binding == "output") {
                    descriptorType = vk::DescriptorType::eStorageImage;
                } else if (bindingTypes.contains(binding)) {
                    descriptorType = bindingTypes.at(binding);
                    count = 1;
                    if (descriptorCounts.contains(binding)) {
                        count = descriptorCounts.at(binding);
                    }
                } else {
                    throw std::runtime_error("Unknown binding type: " + binding);
                }
                pass->addInput(i, descriptorType, count);
            }
            passes.push_back(std::move(pass));
        }
    }

    void ProcessingPipeline::build(VulkanContext &context, DescriptorSetAllocator &allocator, vk::Extent2D screenSize) {
        std::vector<vk::DescriptorSetLayoutBinding> uniformBindings = {
                {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eAll},
                {1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eAll}
        };
        uniformDescriptorSetLayout = context.device->createDescriptorSetLayoutUnique(
                {
                        {},
                        uniformBindings
                }
        );
        uniformDescriptorSet = allocator.allocate(context, *uniformDescriptorSetLayout);

        std::unordered_map<std::string, int32_t> variables;
        variables["width"] = static_cast<int32_t>(screenSize.width);
        variables["height"] = static_cast<int32_t>(screenSize.height);

        images.clear();
        imageArrays.clear();
        for (const auto &desc: imageDescriptions) {
            vk::Extent2D size {
                    static_cast<uint32_t>(parseExpression(desc.width, variables)),
                    static_cast<uint32_t>(parseExpression(desc.height, variables))
            };
            if (desc.count == 1) {
                images[desc.name] = std::make_unique<Image>(context, size, desc.format);
            } else {
                for (auto i = 0; i < desc.count; i++) {
                    imageArrays[desc.name].emplace_back(context, size, desc.format);
                }
            }
        }
        samplers.clear();
        std::unordered_map<std::string, std::string> samplerImages;
        for (const auto &sampler : samplerDescriptions) {
            samplers[sampler.name] = context.device->createSamplerUnique(vk::SamplerCreateInfo{
                    {},
                    sampler.bilinear ? vk::Filter::eLinear : vk::Filter::eNearest,
                    sampler.bilinear ? vk::Filter::eLinear : vk::Filter::eNearest,
                    sampler.bilinear ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest,
                    sampler.clamp ? vk::SamplerAddressMode::eClampToEdge : vk::SamplerAddressMode::eRepeat,
                    sampler.clamp ? vk::SamplerAddressMode::eClampToEdge : vk::SamplerAddressMode::eRepeat,
                    sampler.clamp ? vk::SamplerAddressMode::eClampToEdge : vk::SamplerAddressMode::eRepeat,
            });
            samplerImages[sampler.name] = sampler.image;
        }

        outputImage = std::make_unique<Image>(context, screenSize, vk::Format::eB8G8R8A8Unorm);
        dispatchSizes.resize(passes.size());

        for (size_t i = 0; i < passes.size(); i++) {
            passes[i]->build(context, allocator, *uniformDescriptorSetLayout);
            for (size_t j = 0; j < passDescriptions[i].bindings.size(); j++) {
                const auto binding = passDescriptions[i].bindings[j];
                if (binding == "output") {
                    passes[i]->setInputImage(context, j, *outputImage);
                    continue;
                }
                if (images.contains(binding)) {
                    passes[i]->setInputImage(context, j, *images.at(binding));
                    continue;
                }
                if (imageArrays.contains(binding)) {
                    passes[i]->setInputImageArray(context, j, imageArrays.at(binding));
                    continue;
                }
                if (samplers.contains(binding)) {
                    if (!images.contains(samplerImages.at(binding))) {
                        throw std::runtime_error("Unknown image: " + samplerImages.at(binding));
                    }
                    passes[i]->setInputSampler(context, j, *samplers.at(binding), *images[samplerImages.at(binding)]);
                    continue;
                }
                throw std::runtime_error("Unknown binding: " + binding);
            }

            dispatchSizes[i].width = parseExpression(passDescriptions[i].width, variables);
            dispatchSizes[i].height = parseExpression(passDescriptions[i].height, variables);
            dispatchSizes[i].depth = parseExpression(passDescriptions[i].depth, variables);
        }
    }

    void ProcessingPipeline::dispatch(vk::CommandBuffer commandBuffer) {
        for (size_t i = 0; i < passes.size(); i++) {
            passes[i]->dispatch(
                    commandBuffer,
                    *uniformDescriptorSet,
                    dispatchSizes[i].width,
                    dispatchSizes[i].height,
                    dispatchSizes[i].depth
            );
        }
    }

}  // namespace rendering
