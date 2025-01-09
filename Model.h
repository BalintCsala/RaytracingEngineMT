#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <memory>
#include "tiny_gltf.h"
#include "AccelerationStructure.h"
#include "TextureCache.h"

namespace rendering {
    struct VertexData {
        glm::vec2 uv;
        glm::vec3 normal;
        glm::vec3 tangent;
    };

    class Model {
    public:
        std::unique_ptr<Buffer> positionBuffer;
        std::unique_ptr<Buffer> indexBuffer;
        std::unique_ptr<Buffer> vertexDataBuffer;
        std::unique_ptr<AccelerationStructure> blas;
        int32_t baseColorId;
        int32_t normalId;
        int32_t metallicRoughnessId;
        int32_t emissiveId;
        uint32_t triangleCount;

        Model(VulkanContext &context, const std::vector<glm::vec3> &positions, const std::vector<uint32_t> &indices,
              const std::vector<VertexData> &vertexData, int32_t baseColorId, int32_t normalId,
              int32_t metallicRoughnessId, int32_t emissiveId);

        Model(const Model &) = delete;

        Model &operator=(const Model &) = delete;

        Model(Model &&other) noexcept;

        static Model
        fromGLTFPrimitve(
                VulkanContext &context,
                const std::string &modelPath,
                const std::string &uniquePrimitiveID,
                const tinygltf::Model &model,
                const tinygltf::Primitive &primitive,
                TextureCache &textureCache,
                const glm::mat4 &transform
        );
    };

}