#include "AccelerationStructure.h"

#include "util.h"

namespace rendering {
    AccelerationStructure::AccelerationStructure(
            VulkanContext &context,
            vk::AccelerationStructureGeometryKHR geometry,
            uint32_t primitiveCount,
            vk::AccelerationStructureTypeKHR type
    )
            : geometry(geometry) {
        buildGeometryInfo =
                vk::AccelerationStructureBuildGeometryInfoKHR{
                        type,
                        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
                        vk::BuildAccelerationStructureModeKHR::eBuild,
                        {},
                        {},
                        this->geometry
                };

        auto buildSizesInfo = context.device->getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometryInfo, primitiveCount
        );
        scratchBufferSize = buildSizesInfo.buildScratchSize;

        accelerationStructureBuffer = std::make_unique<Buffer>(
                context,
                buildSizesInfo.accelerationStructureSize,
                vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress
        );

        vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{
                {}, *accelerationStructureBuffer->buffer, {}, buildSizesInfo.accelerationStructureSize, type
        };
        accelerationStructure = context.device->createAccelerationStructureKHRUnique(accelerationStructureCreateInfo);

        buildGeometryInfo.dstAccelerationStructure = *accelerationStructure;

        buildRangeInfo = vk::AccelerationStructureBuildRangeInfoKHR{primitiveCount, 0, 0, 0};

        accelInfo = vk::WriteDescriptorSetAccelerationStructureKHR{1, &*accelerationStructure};

        Buffer buffer = createScratchBuffer(context);
        buildGeometryInfo.scratchData = {alignUp(buffer.deviceAddress(), 128)};
        context.createAndSubmitCommandBuffer(
                [&](vk::CommandBuffer cmd) { cmd.buildAccelerationStructuresKHR(buildGeometryInfo, &buildRangeInfo); },
                false
        );
    }

    Buffer AccelerationStructure::createScratchBuffer(VulkanContext &context) const {
        return {
                context,
                scratchBufferSize + 128,
                vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
        };
    }
}  // namespace rendering
