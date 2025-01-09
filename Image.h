#pragma once

#include "VulkanContext.h"
#include <vk_mem_alloc.hpp>

namespace rendering {

    class Image {
    public:
        vma::UniqueImage image;
        vma::UniqueAllocation allocation;
        vk::UniqueImageView view;
        vk::Extent2D size;

        Image(VulkanContext &context, vk::Extent2D size, vk::Format format, const void *data = nullptr);

        Image(const Image &) = delete;

        Image &operator=(const Image &) = delete;
        Image(Image&& other) noexcept;
        Image& operator=(Image&& other) noexcept;
    };

}