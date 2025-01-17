#include "TextureCache.h"

namespace rendering {
int32_t TextureCache::loadImage(VulkanContext &context,
                                const tinygltf::Model &model,
                                int32_t textureIndex,
                                vk::Format format) {
    if (loadedTextureIndices.contains(textureIndex)) {
        return loadedTextureIndices.at(textureIndex);
    }

    const auto textureData = model.textures[textureIndex];
    const auto imageData = model.images[textureData.source];
    const auto index = static_cast<int32_t>(images.size());
    images.push_back(std::make_unique<Image>(context,
                                             vk::Extent2D{
                                                 static_cast<uint32_t>(imageData.width),
                                                 static_cast<uint32_t>(imageData.height),
                                             },
                                             format,
                                             imageData.image.data()));
    loadedTextureIndices[textureIndex] = index;
    return index;
}

void TextureCache::nextModel() {
    loadedTextureIndices.clear();
}

int32_t TextureCache::create1x1Texture(
    VulkanContext &context, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, vk::Format format) {
    const auto index = static_cast<int32_t>(images.size());
    uint8_t color[] = {red, green, blue, alpha};
    images.push_back(std::make_unique<Image>(context,
                                             vk::Extent2D{
                                                 1,
                                                 1,
                                             },
                                             format,
                                             color));
    return index;
}
}  // namespace rendering
