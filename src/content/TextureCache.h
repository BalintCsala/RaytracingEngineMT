#pragma once

#include <map>
#include <cstdint>
#include <vector>
#include <memory>
#include <glm/vec4.hpp>
#include "Image.h"
#include "tiny_gltf.h"

namespace rendering {

    class TextureCache {
    public:
        std::map<uint32_t, int32_t> loadedTextureIndices;
        std::vector<std::unique_ptr<Image>> images;

        int32_t loadImage(VulkanContext &context, const tinygltf::Model &model, int32_t textureIndex, vk::Format format = vk::Format::eR8G8B8A8Unorm);
        int32_t create1x1Texture(VulkanContext &context, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, vk::Format format = vk::Format::eR8G8B8A8Unorm);

        void nextModel();
    };

} // rendering
