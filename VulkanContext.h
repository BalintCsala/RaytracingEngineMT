#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <functional>

#include "Window.h"
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

namespace rendering {

    constexpr uint32_t FRAMES_IN_FLIGHT = 3;

    struct Frame {
        vk::UniqueCommandPool commandPool;
        vk::UniqueCommandBuffer commandBuffer;
        vk::UniqueSemaphore swapchainSemaphore, renderSemaphore;
        vk::UniqueFence renderFence;
    };

    class VulkanContext {
    public:
        vk::DynamicLoader dl;
        vk::UniqueInstance instance;
        vk::UniqueSurfaceKHR surface;
        vk::UniqueDevice device;
        vk::PhysicalDevice physicalDevice;
        vk::Queue queue;
        vk::UniqueSwapchainKHR swapchain;
        std::vector<vk::Image> swapchainImages;
        vma::UniqueAllocator allocator;

        explicit VulkanContext(const Window &window);

        VulkanContext(const VulkanContext &) = delete;

        VulkanContext &operator=(const VulkanContext &) = delete;

        void createAndSubmitCommandBuffer(const std::function<void(vk::CommandBuffer)> &func, bool waitIdle = true);
        Frame &getFrame(uint32_t index);
        static void transitionImage(vk::CommandBuffer commandBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
        void recreateSwapchain(const Window &window);

    private:
        std::vector<Frame> frames;
        vk::UniqueCommandPool immediateCommandPool;
        vk::UniqueCommandBuffer immediateCommandBuffer;

        static std::vector<const char *> getGLFWExtensions();

        static bool checkLayerSupport(const std::vector<const char *> &requiredLayers);

        [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

    };
}
