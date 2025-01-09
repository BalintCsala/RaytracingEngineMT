#include "Image.h"

#include "Buffer.h"

rendering::Image::Image(VulkanContext &context, vk::Extent2D size, vk::Format format, const void *data)
        : size(size) {
    const vk::ImageCreateInfo createInfo{
            {},
            vk::ImageType::e2D,
            format,
            vk::Extent3D{size.width, size.height, 1},
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled,
            vk::SharingMode::eExclusive,
    };

    constexpr vma::AllocationCreateInfo allocationCreateInfo{
            vma::AllocationCreateFlagBits::eDedicatedMemory,
            vma::MemoryUsage::eAuto,
    };

    auto [img, alloc] = context.allocator->createImageUnique(createInfo, allocationCreateInfo);
    image = std::move(img);
    allocation = std::move(alloc);

    if (data) {
        uint32_t bytesPerPixel;
        switch (format) {
            case vk::Format::eR8G8B8A8Unorm:
            case vk::Format::eR8G8B8A8Srgb:
                bytesPerPixel = 4;
                break;
            default:
                throw std::runtime_error(std::format("Unhandled texture format: %s", vk::to_string(format)));
        }
        const auto bytes = size.width * size.height * bytesPerPixel;
        Buffer buffer(context, bytes, vk::BufferUsageFlagBits::eTransferSrc, data);
        context.createAndSubmitCommandBuffer(
                [&](vk::CommandBuffer cmd) {
                    vk::BufferImageCopy region{
                            0,
                            0,
                            0,
                            vk::ImageSubresourceLayers{
                                    vk::ImageAspectFlagBits::eColor,
                                    0,
                                    0,
                                    1,
                            },
                            {},
                            vk::Extent3D{
                                    size.width,
                                    size.height,
                                    1,
                            },
                    };
                    rendering::VulkanContext::transitionImage(
                            cmd, *image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal
                    );
                    cmd.copyBufferToImage(*buffer.buffer, *image, vk::ImageLayout::eTransferDstOptimal, {region});

                    rendering::VulkanContext::transitionImage(
                            cmd, *image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral
                    );
                },
                false
        );
    } else {
        context.createAndSubmitCommandBuffer(
                [&](vk::CommandBuffer cmd) {
                    rendering::VulkanContext::transitionImage(
                            cmd, *image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral
                    );
                },
                false
        );
    }

    const vk::ImageViewCreateInfo viewCreateInfo{
            {},
            *image,
            vk::ImageViewType::e2D,
            format,
            vk::ComponentMapping{
                    vk::ComponentSwizzle::eIdentity,
                    vk::ComponentSwizzle::eIdentity,
                    vk::ComponentSwizzle::eIdentity,
                    vk::ComponentSwizzle::eIdentity,
            },
            vk::ImageSubresourceRange{
                    vk::ImageAspectFlagBits::eColor,
                    0, 1,
                    0, 1,
            },
    };
    view = context.device->createImageViewUnique(viewCreateInfo);
}

rendering::Image::Image(rendering::Image &&other) noexcept
        : image(std::move(other.image)),
          allocation(std::move(other.allocation)),
          view(std::move(other.view)),
          size(other.size) {
}

rendering::Image &rendering::Image::operator=(rendering::Image &&other) noexcept {
    image = std::move(other.image);
    allocation = std::move(other.allocation);
    view = std::move(other.view);
    size = other.size;
    return *this;
}
