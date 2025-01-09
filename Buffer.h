#pragma once

#include "VulkanContext.h"

#include <optional>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

namespace rendering {

    class Buffer {
    public:
        vma::UniqueBuffer buffer;
        vma::UniqueAllocation allocation;

        Buffer(const VulkanContext &context, uint32_t size, vk::BufferUsageFlags usage, const void *data = nullptr);

        Buffer(const Buffer &) = delete;

        Buffer &operator=(const Buffer &) = delete;

        Buffer(Buffer &&other) noexcept;

        void updateData(const VulkanContext &context, uint32_t size, const void *data);

        vk::DeviceAddress deviceAddress();
    private:
        std::optional<vk::DeviceAddress> _deviceAddress;

    };

}
