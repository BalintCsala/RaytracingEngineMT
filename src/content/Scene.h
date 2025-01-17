#pragma once

#include "VulkanContext.h"
#include "tiny_gltf.h"
#include "Model.h"
#include <glm/mat4x4.hpp>
#include <thread>

namespace rendering {

    struct Object {
        uint32_t modelId;
        uint32_t shaderId;
        glm::mat4 transform;
    };

    struct ObjDesc {
        glm::mat4 modelMatrix;
        uint32_t triangleCount;
        int32_t baseColorId;
        int32_t normalId;
        int32_t metallicRoughnessId;
        int32_t emissiveId;
        uint64_t indexAddress;
        uint64_t positionAddress;
        uint64_t vertexDataAddress;
    };

    class Scene {
    public:
        std::unique_ptr<AccelerationStructure> accelerationStructure;
        TextureCache textureCache;
        std::unique_ptr<Buffer> objDescriptorBuffer;
        std::unique_ptr<Buffer> emissiveObjectIdsBuffer;
        std::vector<Object> objects;
        std::vector<std::string> shaderPaths;
        std::vector<uint32_t> emissiveObjectIds;
        vk::UniqueSampler sampler;

        Scene() = default;

        Scene(const Scene &) = delete;

        Scene &operator=(const Scene &) = delete;

        Scene(Scene &&other) noexcept;

        void loadGLTF(rendering::VulkanContext &context, const std::string &path, int32_t sceneId,
                      glm::mat4 transform = glm::mat4(1.0f), int32_t shaderId = 0u);

        void addObject(uint32_t modelId, uint32_t shaderId, const glm::mat4 &transform);

        uint32_t addModel(Model &model);

        void build(VulkanContext &context);

    private:
        std::vector<Model> models;
        std::unique_ptr<Buffer> instanceBuffer;

        void addNode(
                VulkanContext &context,
                const std::string &path,
                const tinygltf::Model &model,
                uint32_t nodeId,
                glm::mat4 parentTransform,
                std::map<uint32_t, std::vector<uint32_t>> &modelMap,
                int32_t shaderId,
                int32_t &count
        );

    };

} // rendering