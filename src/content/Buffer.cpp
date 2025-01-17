#include "Buffer.h"

namespace rendering {

Buffer::Buffer(const VulkanContext &context, uint32_t size, vk::BufferUsageFlags usage, const void *data) {
    const vk::BufferCreateInfo bufferCreateInfo{
        {},
        size,
        usage,
        vk::SharingMode::eExclusive,
    };
    constexpr vma::AllocationCreateInfo allocationCreateInfo{
        {},
        vma::MemoryUsage::eCpuToGpu,
    };
    auto [buf, alloc] = context.allocator->createBufferUnique(bufferCreateInfo, allocationCreateInfo);
    buffer = std::move(buf);
    allocation = std::move(alloc);

    if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        const vk::BufferDeviceAddressInfo addressInfo{*buffer};
        _deviceAddress = context.device->getBufferAddress(addressInfo);
    }

    if (data) {
        updateData(context, size, data);
    }
}

void Buffer::updateData(const VulkanContext &context, uint32_t size, const void *data) {
    void *mapped = context.allocator->mapMemory(*allocation);
    memcpy(mapped, data, size);
    context.allocator->unmapMemory(*allocation);
}

Buffer::Buffer(Buffer &&other) noexcept
    : buffer(std::move(other.buffer)), allocation(std::move(other.allocation)), _deviceAddress(other._deviceAddress) {}

vk::DeviceAddress Buffer::deviceAddress() {
    if (!_deviceAddress.has_value()) {
        throw std::runtime_error(
            "Tried to access the device address of a buffer without the device address usage flag");
    }
    return _deviceAddress.value();
}
}  // namespace rendering
