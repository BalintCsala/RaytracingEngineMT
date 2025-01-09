#pragma once

#include "VulkanContext.h"

#include <memory>
#include <vulkan/vulkan.hpp>

#include "Buffer.h"

namespace rendering {

    class AccelerationStructure {
    public:
        std::unique_ptr<Buffer> accelerationStructureBuffer;
        vk::UniqueAccelerationStructureKHR accelerationStructure;
        vk::WriteDescriptorSetAccelerationStructureKHR accelInfo;
        vk::AccelerationStructureGeometryKHR geometry;
        vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo;
        uint32_t scratchBufferSize;

        AccelerationStructure(
                VulkanContext &context,
                vk::AccelerationStructureGeometryKHR geometry,
                uint32_t primitiveCount,
                vk::AccelerationStructureTypeKHR type
        );

        Buffer createScratchBuffer(VulkanContext &context) const;

        AccelerationStructure(const AccelerationStructure &) = delete;

        AccelerationStructure &operator=(const AccelerationStructure &) = delete;
    };

}
