#ifndef DIPTERV_RT_UTIL_H
#define DIPTERV_RT_UTIL_H

#include <cstdint>
#include <fstream>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/quaternion.hpp>

#include "tiny_gltf.h"

inline std::vector<uint32_t> readShaderFile(const std::string &path) {
    std::ifstream inp(path, std::ios::ate | std::ios::binary);
    if (!inp.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    const auto size = inp.tellg();
    std::vector<uint32_t> data(static_cast<uint32_t>(size) / 4);
    inp.seekg(0);
    inp.read(reinterpret_cast<char *>(data.data()), size);
    inp.close();
    return data;
}

inline vk::UniqueShaderModule createShader(const rendering::VulkanContext &context, const std::string &path) {
    const auto code = readShaderFile(path);
    vk::ShaderModuleCreateInfo createInfo{
        {},
        code,
    };
    return context.device->createShaderModuleUnique(createInfo);
}

inline uint64_t alignUp(uint64_t size, uint64_t alignment) {
    return (size + (alignment - 1u)) & (~(alignment - 1u));
}

template <typename In, typename Out>
inline void convertGltfType(In &in, Out &out) {
    std::transform(
        in.begin(), in.end(), reinterpret_cast<float *>(&out), [&](const auto &x) { return static_cast<float>(x); });
}

inline glm::mat4 getLocalTransform(const tinygltf::Node &node) {
    glm::mat4 localTransform(1.0f);
    if (!node.matrix.empty()) {
        convertGltfType(node.matrix, localTransform);
    }
    if (!node.translation.empty()) {
        glm::vec3 translation(0.0);
        convertGltfType(node.translation, translation);
        localTransform *= glm::translate(glm::mat4(1.0f), translation);
    }
    if (!node.rotation.empty()) {
        glm::quat rotation(0.0, 0.0, 0.0, 1.0);
        convertGltfType(node.rotation, rotation);
        localTransform *= glm::toMat4(rotation);
    }
    if (!node.scale.empty()) {
        glm::vec3 scale(0.0);
        convertGltfType(node.scale, scale);
        localTransform *= glm::scale(glm::mat4(1.0f), scale);
    }
    return localTransform;
}

#endif  // DIPTERV_RT_UTIL_H
