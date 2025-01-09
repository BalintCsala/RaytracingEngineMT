#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>
#include <thread>
#include <utility>
#include <iostream>

#include "util.h"

tinygltf::Model loadBinaryGLTFModel(tinygltf::TinyGLTF &loader, const std::string &path) {
    std::string err, warn;
    tinygltf::Model model;
    bool loadResult = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    if (!loadResult) {
        throw std::runtime_error("Failed to parse the sponza GLTF file");
    }
    if (!warn.empty()) {
        std::cout << warn << "\n";
    }
    if (!err.empty()) {
        throw std::runtime_error(err);
    }
    return model;
}

tinygltf::Model loadASCIIGLTFModel(tinygltf::TinyGLTF &loader, const std::string &path) {
    std::string err, warn;
    tinygltf::Model model;
    bool loadResult = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    if (!loadResult) {
        throw std::runtime_error("Failed to parse the sponza GLTF file");
    }
    if (!warn.empty()) {
        std::cout << warn << "\n";
    }
    if (!err.empty()) {
        throw std::runtime_error(err);
    }
    return model;
}

namespace rendering {
void Scene::loadGLTF(rendering::VulkanContext &context,
                     const std::string &path,
                     int32_t sceneId,
                     glm::mat4 transform,
                     int32_t shaderId) {


    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    if (path.ends_with(".glb")) {
        model = loadBinaryGLTFModel(loader, path);
    } else {
        model = loadASCIIGLTFModel(loader, path);
    }

    textureCache.nextModel();
    if (sceneId == -1) {
        sceneId = model.defaultScene;
    }
    std::map<uint32_t, std::vector<uint32_t>> modelMap;
    const auto scene = model.scenes[sceneId];
    int32_t count = 0;
    for (const auto &nodeId : scene.nodes) {
        addNode(context, path, model, nodeId, transform, modelMap, shaderId, count);
    }
    context.device->waitIdle();
}

void Scene::addNode(
        VulkanContext &context,
        const std::string &path,
        const tinygltf::Model &model,
        uint32_t nodeId,
        glm::mat4 parentTransform,
        std::map<uint32_t, std::vector<uint32_t>> &modelMap,
        int32_t shaderId,
        int32_t &count
) {
    const auto node = model.nodes[nodeId];
    glm::mat4 localTransform = getLocalTransform(node);
    const auto transform = parentTransform * localTransform;
    if (node.mesh >= 0) {
        if (!modelMap.contains(node.mesh)) {
            std::vector<uint32_t> modelIds;
            const auto mesh = model.meshes[node.mesh];
            for (size_t i = 0; i < mesh.primitives.size(); i++) {
                const auto &primitive = mesh.primitives[i];
                count++;
                std::cout << "Creating model " << count << "\n";
                auto m = Model::fromGLTFPrimitve(
                        context,
                        path,
                        std::to_string(node.mesh) + "_" + std::to_string(i),
                        model,
                        primitive,
                        textureCache,
                        transform
                );
                modelIds.push_back(addModel(m));
            }
            modelMap[node.mesh] = modelIds;
        }
        for (const auto &modelId : modelMap.at(node.mesh)) {
            addObject(modelId, shaderId, transform);
        }
    }
    std::ranges::for_each(node.children,
                          [&](const auto childId) {
                              addNode(
                                      context,
                                      path,
                                      model,
                                      childId,
                                      transform,
                                      modelMap,
                                      0,
                                      count
                              ); });
}

void Scene::addObject(uint32_t modelId, uint32_t shaderId, const glm::mat4 &transform) {
    if (models[modelId].emissiveId >= 0) {
        emissiveObjectIds.push_back(objects.size());
    }
    objects.push_back(Object{
        .modelId = modelId,
        .shaderId = shaderId,
        .transform = transform,
    });
}

uint32_t Scene::addModel(Model &model) {
    const auto index = models.size();
    models.push_back(std::move(model));
    return index;
}

void Scene::build(VulkanContext &context) {
    // std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
    // std::vector<vk::AccelerationStructureBuildRangeInfoKHR *> buildRangeInfos;
    // std::vector<Buffer> scratchBuffers;
    // buildGeometryInfos.reserve(models.size());
    // buildRangeInfos.reserve(models.size());
    /*for (const auto &m: models) {
        Buffer buffer = m.blas->createScratchBuffer(context);
        m.blas->buildGeometryInfo.scratchData = {.deviceAddress = utils::alignUp(buffer.deviceAddress(), 128)};
        //buildGeometryInfos.push_back(m.blas->buildGeometryInfo);
        //buildRangeInfos.push_back(&m.blas->buildRangeInfo);
        //scratchBuffers.push_back(std::move(buffer));
        context.createAndSubmitCommandBuffer([&](vk::CommandBuffer cmd) {
            cmd.buildAccelerationStructuresKHR(m.blas->buildGeometryInfo, &m.blas->buildRangeInfo);
        });
    }*/

    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    for (const auto &object : objects) {
        const auto t = object.transform;
        const auto instanceTransform = vk::TransformMatrixKHR{
            std::array{
                       std::array{t[0][0], t[1][0], t[2][0], t[3][0]},
                       std::array{t[0][1], t[1][1], t[2][1], t[3][1]},
                       std::array{t[0][2], t[1][2], t[2][2], t[3][2]},
                       }
        };
        vk::AccelerationStructureInstanceKHR instance{
            instanceTransform,
            {},
            0xFF,
            object.shaderId,
            {},
            models.at(object.modelId).blas->accelerationStructureBuffer->deviceAddress(),
        };
        instance.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
        instances.push_back(instance);
    }

    instanceBuffer = std::make_unique<Buffer>(context,
                                              instances.size() * sizeof(vk::AccelerationStructureInstanceKHR),
                                              vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                                                  vk::BufferUsageFlagBits::eStorageBuffer |
                                                  vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                              instances.data());

    vk::AccelerationStructureGeometryInstancesDataKHR instanceData{
        false,
        {instanceBuffer->deviceAddress()},
    };

    vk::AccelerationStructureGeometryKHR instanceGeometry{
            vk::GeometryTypeKHR::eInstances,
                                                          vk::AccelerationStructureGeometryDataKHR{instanceData},
                                                          vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation
    };

    accelerationStructure = std::make_unique<AccelerationStructure>(
        context, instanceGeometry, instances.size(), vk::AccelerationStructureTypeKHR::eTopLevel);

    std::vector<ObjDesc> descriptors;
    for (const auto &obj : objects) {
        descriptors.push_back(ObjDesc{obj.transform,
                                      models[obj.modelId].triangleCount,
                                      models[obj.modelId].baseColorId,
                                      models[obj.modelId].normalId,
                                      models[obj.modelId].metallicRoughnessId,
                                      models[obj.modelId].emissiveId,
                                      models[obj.modelId].indexBuffer->deviceAddress(),
                                      models[obj.modelId].positionBuffer->deviceAddress(),
                                      models[obj.modelId].vertexDataBuffer->deviceAddress()});
    }
    objDescriptorBuffer = std::make_unique<Buffer>(
        context, descriptors.size() * sizeof(ObjDesc), vk::BufferUsageFlagBits::eStorageBuffer, descriptors.data());

    emissiveObjectIdsBuffer = std::make_unique<Buffer>(
        context,
        sizeof(uint32_t) * emissiveObjectIds.size(),
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        emissiveObjectIds.data());

    const vk::SamplerCreateInfo samplerCreateInfo{{},
                                                  vk::Filter::eLinear,
                                                  vk::Filter::eLinear,
                                                  vk::SamplerMipmapMode::eLinear,
                                                  vk::SamplerAddressMode::eRepeat,
                                                  vk::SamplerAddressMode::eRepeat,
                                                  vk::SamplerAddressMode::eRepeat};
    sampler = context.device->createSamplerUnique(samplerCreateInfo);
}

Scene::Scene(Scene &&other) noexcept
    : accelerationStructure(std::move(other.accelerationStructure)),
      textureCache(std::move(other.textureCache)),
      objDescriptorBuffer(std::move(other.objDescriptorBuffer)),
      objects(std::move(other.objects)),
      shaderPaths(std::move(other.shaderPaths)),
      models(std::move(other.models)),
      instanceBuffer(std::move(other.instanceBuffer)) {}

}  // namespace rendering
