#include "Model.h"

#include <glm/detail/type_mat3x3.hpp>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <numeric>
#include <fstream>
#include <filesystem>

namespace rendering {

template <typename T>
std::vector<T> readDataFromAccessor(const tinygltf::Model &model, uint32_t accessorIndex) {
    const auto accessor = model.accessors[accessorIndex];
    const auto bufferView = model.bufferViews[accessor.bufferView];
    const auto buffer = model.buffers[bufferView.buffer];
    const auto stride = accessor.ByteStride(bufferView);

    std::vector<T> result;
    result.reserve(accessor.count);
    auto offset = bufferView.byteOffset + accessor.byteOffset;
    for (size_t i = 0; i < accessor.count; i++) {
        const auto *data = reinterpret_cast<const T *>(buffer.data.data() + offset);
        offset += stride;
        result.push_back(*data);
    }
    return result;
}

std::vector<uint32_t> readIndicesAccessor(const tinygltf::Model &model, uint32_t accessorIndex) {
    const auto accessor = model.accessors[accessorIndex];
    const auto bufferView = model.bufferViews[accessor.bufferView];
    const auto buffer = model.buffers[bufferView.buffer];

    auto stride = accessor.ByteStride(bufferView);

    std::vector<uint32_t> result;
    result.reserve(accessor.count);
    auto offset = bufferView.byteOffset + accessor.byteOffset;
    for (size_t i = 0; i < accessor.count; i++) {
        if (stride == 2) {
            const auto *data = reinterpret_cast<const uint16_t *>(buffer.data.data() + offset);
            result.push_back(static_cast<uint32_t>(*data));
        } else {
            const auto *data = reinterpret_cast<const uint32_t *>(buffer.data.data() + offset);
            result.push_back(*data);
        }
        offset += stride;
    }
    return result;
}

Model::Model(VulkanContext &context,
             const std::vector<glm::vec3> &positions,
             const std::vector<uint32_t> &indices,
             const std::vector<VertexData> &vertexData,
             int32_t baseColorId,
             int32_t normalId,
             int32_t metallicRoughnessId,
             int32_t emissiveId)
    : baseColorId(baseColorId),
      normalId(normalId),
      metallicRoughnessId(metallicRoughnessId),
      emissiveId(emissiveId),
      triangleCount(indices.size() / 3) {
    positionBuffer = std::make_unique<Buffer>(context,
                                              positions.size() * sizeof(glm::vec3),
                                              vk::BufferUsageFlagBits::eShaderDeviceAddress |
                                                  vk::BufferUsageFlagBits::eStorageBuffer |
                                                  vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                                              positions.data());
    indexBuffer = std::make_unique<Buffer>(context,
                                           indices.size() * sizeof(uint32_t),
                                           vk::BufferUsageFlagBits::eShaderDeviceAddress |
                                               vk::BufferUsageFlagBits::eStorageBuffer |
                                               vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                                           indices.data());
    vertexDataBuffer = std::make_unique<Buffer>(
        context,
        vertexData.size() * sizeof(VertexData),
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer,
        vertexData.data());

    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData{
        vk::Format::eR32G32B32Sfloat,
        {positionBuffer->deviceAddress()},
        sizeof(glm::vec3),
        50000,
        vk::IndexType::eUint32,
        {
                                    indexBuffer->deviceAddress(),
                                    },
    };

    vk::AccelerationStructureGeometryKHR geometry{
        vk::GeometryTypeKHR::eTriangles,
        vk::AccelerationStructureGeometryDataKHR{
                                                 trianglesData,
         },
        vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation,
    };

    blas = std::make_unique<AccelerationStructure>(
        context, geometry, indices.size() / 3, vk::AccelerationStructureTypeKHR::eBottomLevel);
}

Model Model::fromGLTFPrimitve(
        VulkanContext &context,
        const std::string &modelPath,
        const std::string &uniquePrimitiveID,
        const tinygltf::Model &model,
        const tinygltf::Primitive &primitive,
        TextureCache &textureCache,
        const glm::mat4 &transform
) {
    auto texcoordIndex = -1;
    auto baseColorId = -1;
    auto normalId = -1;
    auto metallicRoughnessId = -1;
    auto emissiveId = -1;
    if (primitive.material >= 0) {
        const auto &material = model.materials[primitive.material];
        texcoordIndex = material.pbrMetallicRoughness.baseColorTexture.texCoord;
        if (material.pbrMetallicRoughness.baseColorTexture.index > 0) {
            baseColorId = textureCache.loadImage(
                context, model, material.pbrMetallicRoughness.baseColorTexture.index, vk::Format::eR8G8B8A8Unorm);
        } else if (!material.pbrMetallicRoughness.baseColorFactor.empty()) {
            auto red = static_cast<int8_t>(material.pbrMetallicRoughness.baseColorFactor[0] * 255.0);
            auto green = static_cast<int8_t>(material.pbrMetallicRoughness.baseColorFactor[1] * 255.0);
            auto blue = static_cast<int8_t>(material.pbrMetallicRoughness.baseColorFactor[2] * 255.0);
            baseColorId = textureCache.create1x1Texture(context, red, green, blue, 255, vk::Format::eR8G8B8A8Unorm);
        }

        if (material.normalTexture.index >= 0) {
            normalId = textureCache.loadImage(context, model, material.normalTexture.index);
        }

        if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
            metallicRoughnessId =
                textureCache.loadImage(context, model, material.pbrMetallicRoughness.metallicRoughnessTexture.index);
        } else {
            auto roughness = static_cast<int8_t>(material.pbrMetallicRoughness.roughnessFactor * 255.0);
            auto metallic = static_cast<int8_t>(material.pbrMetallicRoughness.roughnessFactor * 255.0);
            metallicRoughnessId = textureCache.create1x1Texture(context, 0, roughness, metallic, 255);
        }

        if (material.emissiveTexture.index >= 0) {
            emissiveId = textureCache.loadImage(context, model, material.emissiveTexture.index);
        } else if (!material.emissiveFactor.empty() && (material.emissiveFactor[0] != 0.0 || material.emissiveFactor[1] != 0.0 || material.emissiveFactor[2] != 0.0)) {
            auto red = static_cast<int8_t>(material.emissiveFactor[0] * 255.0);
            auto green = static_cast<int8_t>(material.emissiveFactor[1] * 255.0);
            auto blue = static_cast<int8_t>(material.emissiveFactor[2] * 255.0);
            emissiveId = textureCache.create1x1Texture(context, red, green, blue, 255);
        }
    }

    const auto cacheFilePath = "models-cache/" + modelPath + "/" + uniquePrimitiveID + ".dat";
    if (std::filesystem::exists(cacheFilePath)) {
        std::ifstream cacheFile(cacheFilePath, std::ios::binary);
        std::istreambuf_iterator<char> it(cacheFile);

        size_t positionsSize, indicesSize, vertexDataSize;
        cacheFile.read(reinterpret_cast<char *>(&positionsSize), sizeof(size_t));
        cacheFile.read(reinterpret_cast<char *>(&indicesSize), sizeof(size_t));
        cacheFile.read(reinterpret_cast<char *>(&vertexDataSize), sizeof(size_t));

        std::vector<glm::vec3> positions;
        std::vector<uint32_t> indices;
        std::vector<VertexData> vertexData;
        positions.resize(positionsSize);
        indices.resize(indicesSize);
        vertexData.resize(vertexDataSize);

        cacheFile.read(reinterpret_cast<char *>(positions.data()), static_cast<uint32_t>(positionsSize * sizeof(glm::vec3)));
        cacheFile.read(reinterpret_cast<char *>(indices.data()), static_cast<uint32_t>(indicesSize * sizeof(uint32_t)));
        cacheFile.read(reinterpret_cast<char *>(vertexData.data()), static_cast<uint32_t>(vertexDataSize * sizeof(VertexData)));

        return {context, positions, indices, vertexData, baseColorId, normalId, metallicRoughnessId, emissiveId};
    }

    std::vector positions = readDataFromAccessor<glm::vec3>(model, primitive.attributes.at("POSITION"));
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangents;
    std::vector<uint32_t> indices;
    const auto texcoordName = std::format("TEXCOORD_{}", texcoordIndex);

    if (primitive.attributes.contains(texcoordName)) {
        uvs = readDataFromAccessor<glm::vec2>(model, primitive.attributes.at(texcoordName));
    }
    if (primitive.attributes.contains("NORMAL")) {
        normals = readDataFromAccessor<glm::vec3>(model, primitive.attributes.at("NORMAL"));
    }
    if (primitive.attributes.contains("TANGENT")) {
        tangents = readDataFromAccessor<glm::vec3>(model, primitive.attributes.at("TANGENT"));
    }

    if (primitive.indices >= 0) {
        indices = readIndicesAccessor(model, primitive.indices);
    } else {
        indices.resize(positions.size());
        // Fill in the indices with sequential numbers
        std::iota(indices.begin(), indices.end(), 0);
    }

    glm::mat3 normalMatrix = glm::inverse(glm::transpose(glm::mat3(transform)));

    std::vector<VertexData> vertexData;
    for (size_t i = 0; i < positions.size(); i++) {
        vertexData.push_back(VertexData{
            uvs.size() > i ? uvs[i] : glm::vec2(0.0, 0.0),
            normalMatrix * (normals.size() > i ? normals[i] : glm::vec3(0.0f, 0.0f, 0.0f)),
            normalMatrix * (tangents.size() > i ? tangents[i] : glm::vec3(0.0f, 0.0f, 0.0f))
        });
    }

    std::filesystem::create_directories("models-cache/" + modelPath + "/");
    std::ofstream cacheFile(cacheFilePath, std::ios::binary);

    size_t positionsSize = positions.size();
    size_t indicesSize = indices.size();
    size_t vertexDataSize = vertexData.size();

    cacheFile.write(reinterpret_cast<const char *>(&positionsSize), sizeof(size_t));
    cacheFile.write(reinterpret_cast<const char *>(&indicesSize), sizeof(size_t));
    cacheFile.write(reinterpret_cast<const char *>(&vertexDataSize), sizeof(size_t));

    cacheFile.write(reinterpret_cast<const char *>(positions.data()), static_cast<int32_t>(sizeof(positions[0]) * positions.size()));
    cacheFile.write(reinterpret_cast<const char *>(indices.data()), static_cast<int32_t>(sizeof(indices[0]) * indices.size()));
    cacheFile.write(reinterpret_cast<const char *>(vertexData.data()), static_cast<int32_t>(sizeof(vertexData[0]) * vertexData.size()));

    return {context, positions, indices, vertexData, baseColorId, normalId, metallicRoughnessId, emissiveId};
}

Model::Model(Model &&other) noexcept
    : positionBuffer(std::move(other.positionBuffer)),
      indexBuffer(std::move(other.indexBuffer)),
      vertexDataBuffer(std::move(other.vertexDataBuffer)),
      blas(std::move(other.blas)),
      baseColorId(other.baseColorId),
      normalId(other.normalId),
      metallicRoughnessId(other.metallicRoughnessId),
      emissiveId(other.emissiveId),
      triangleCount(other.triangleCount) {}

}  // namespace rendering
