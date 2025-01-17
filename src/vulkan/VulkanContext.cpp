#include "VulkanContext.h"

#include <algorithm>
#include <optional>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "glfw_include.h"


VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace rendering {

    VulkanContext::VulkanContext(const Window &window) {
        auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        const std::vector layers{"VK_LAYER_KHRONOS_validation"};

        if (!checkLayerSupport(layers)) {
            throw std::runtime_error("Required layers are not supported");
        }

        auto extensions = getGLFWExtensions();
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

        const vk::ApplicationInfo appInfo{"Diplomaterv", 1, "Nothing", 1, VK_API_VERSION_1_3};

        const vk::InstanceCreateInfo instanceCreateInfo{
                {},
                &appInfo,
                static_cast<uint32_t>(layers.size()),
                layers.data(),
                static_cast<uint32_t>(extensions.size()),
                extensions.data(),
        };

        instance = vk::createInstanceUnique(instanceCreateInfo);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

        VkSurfaceKHR rawSurface;
        glfwCreateWindowSurface(*instance, window.handle, nullptr, &rawSurface);
        surface = vk::UniqueSurfaceKHR(rawSurface, {*instance});

        const auto physicalDevices = instance->enumeratePhysicalDevices();
        physicalDevice = *std::ranges::find_if(
                physicalDevices, [&](const auto &phDevice) {
                    const auto &properties = phDevice.getProperties();
                    return properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
                }
        );

        const auto familyProperties = physicalDevice.getQueueFamilyProperties();
        std::optional<uint32_t> queueFamily;
        for (size_t i = 0; i < familyProperties.size(); i++) {
            const auto supportsCompute = familyProperties[i].queueFlags & vk::QueueFlagBits::eCompute;
            const auto supportsPresent = physicalDevice.getSurfaceSupportKHR(i, surface.get());

            if (supportsCompute && supportsPresent) {
                queueFamily = i;
                break;
            }
        }
        if (!queueFamily.has_value()) {
            throw std::runtime_error("Missing queue family");
        }

        constexpr auto queuePriority = 1.0f;
        const vk::DeviceQueueCreateInfo queueCreateInfo{{}, queueFamily.value(), 1, &queuePriority};

        const std::vector deviceExtensions{
                VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        vk::PhysicalDeviceVulkan13Features vulkan13Features{};
        vulkan13Features.setSynchronization2(true);

        vk::PhysicalDeviceVulkan12Features vulkan12Features{};
        vulkan12Features.setUniformAndStorageBuffer8BitAccess(true)
                .setShaderSampledImageArrayNonUniformIndexing(true)
                .setRuntimeDescriptorArray(true)
                .setScalarBlockLayout(true)
                .setBufferDeviceAddress(true);
        vk::PhysicalDeviceVulkan11Features vulkan11Features;
        vulkan11Features.setStorageBuffer16BitAccess(true).setUniformAndStorageBuffer16BitAccess(true);
        vk::PhysicalDeviceFeatures vulkan10Features{};
        vk::PhysicalDeviceFeatures2 features{
                vulkan10Features,
        };

        vk::DeviceCreateInfo deviceCreateInfo{
                {},
                queueCreateInfo,
                layers,
                deviceExtensions,
        };

        vk::StructureChain<vk::DeviceCreateInfo,
                vk::PhysicalDeviceFeatures2,
                vk::PhysicalDeviceVulkan11Features,
                vk::PhysicalDeviceVulkan12Features,
                vk::PhysicalDeviceVulkan13Features,
                vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
                vk::PhysicalDeviceAccelerationStructureFeaturesKHR>
                createInfoChain{
                deviceCreateInfo,
                features,
                vulkan11Features,
                vulkan12Features,
                vulkan13Features,
                {true},
                {true},
        };

        device = physicalDevice.createDeviceUnique(createInfoChain.get());

        VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

        queue = device->getQueue(queueFamily.value(), 0);

        vk::CommandPoolCreateInfo commandPoolCreateInfo{
                vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                queueFamily.value(),
        };
        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
            Frame frame;
            frame.commandPool = device->createCommandPoolUnique(commandPoolCreateInfo);
            const vk::CommandBufferAllocateInfo allocInfo{*frame.commandPool, vk::CommandBufferLevel::ePrimary, 1};
            frame.commandBuffer = std::move(device->allocateCommandBuffersUnique(allocInfo).front());

            vk::SemaphoreCreateInfo semaphoreCreateInfo{};
            vk::FenceCreateInfo fenceCreateInfo{vk::FenceCreateFlagBits::eSignaled};
            frame.swapchainSemaphore = device->createSemaphoreUnique(semaphoreCreateInfo);
            frame.renderSemaphore = device->createSemaphoreUnique(semaphoreCreateInfo);
            frame.renderFence = device->createFenceUnique(fenceCreateInfo);

            frames.push_back(std::move(frame));
        }

        immediateCommandPool = device->createCommandPoolUnique(commandPoolCreateInfo);
        const vk::CommandBufferAllocateInfo allocInfo{*immediateCommandPool, vk::CommandBufferLevel::ePrimary, 1};
        immediateCommandBuffer = std::move(device->allocateCommandBuffersUnique(allocInfo).front());

        int32_t width, height;
        glfwGetWindowSize(window.handle, &width, &height);

        recreateSwapchain(window);

        vma::AllocatorCreateInfo allocatorCreateInfo{
                vma::AllocatorCreateFlagBits::eBufferDeviceAddress,
                physicalDevice,
                device.get(),
                {},
                {},
                {},
                {},
                {},
                instance.get(),
        };

        allocator = vma::createAllocatorUnique(allocatorCreateInfo);
    }

    std::vector<const char *> VulkanContext::getGLFWExtensions() {
        uint32_t glfwExtensionCount;
        const char **glfwExtensionsRaw = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        return {glfwExtensionsRaw, glfwExtensionsRaw + glfwExtensionCount};
    }

    bool VulkanContext::checkLayerSupport(const std::vector<const char *> &requiredLayers) {
        const std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
        return std::ranges::all_of(
                requiredLayers, [&](const char *requiredLayer) {
                    return std::ranges::any_of(
                            availableLayers, [&](const vk::LayerProperties &layer) {
                                return strcmp(layer.layerName, requiredLayer) == 0;
                            }
                    );
                }
        );
    }

    uint32_t VulkanContext::findMemoryType(const uint32_t typeFilter, const vk::MemoryPropertyFlags properties) const {
        const auto memoryProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }

    void
    VulkanContext::createAndSubmitCommandBuffer(const std::function<void(vk::CommandBuffer)> &func, bool waitIdle) {
        immediateCommandBuffer->reset();
        vk::CommandBufferBeginInfo beginInfo{
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        };
        immediateCommandBuffer->begin(beginInfo);
        func(*immediateCommandBuffer);
        immediateCommandBuffer->end();

        const vk::SubmitInfo submitInfo{
                {},
                {},
                *immediateCommandBuffer,
        };
        queue.submit(submitInfo);
        queue.waitIdle();
    }

    Frame &VulkanContext::getFrame(uint32_t index) {
        return frames[index % FRAMES_IN_FLIGHT];
    }

    void VulkanContext::transitionImage(
            vk::CommandBuffer commandBuffer,
            vk::Image image,
            vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout
    ) {
        const vk::ImageMemoryBarrier2 barrier{
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eMemoryWrite,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                oldLayout,
                newLayout,
                {},
                {},
                image,
                {
                        newLayout == vk::ImageLayout::eDepthAttachmentOptimal ? vk::ImageAspectFlagBits::eDepth
                                                                              : vk::ImageAspectFlagBits::eColor,
                        0, vk::RemainingMipLevels,
                        0, vk::RemainingArrayLayers,
                },
        };

        const vk::DependencyInfo dependencyInfo{
                {},
                {},
                {},
                barrier,
        };

        commandBuffer.pipelineBarrier2(dependencyInfo);
    }

    void VulkanContext::recreateSwapchain(const Window &window) {
        const auto size = window.getSize();
        const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
        vk::SwapchainCreateInfoKHR swapchainCreateInfo{
                {},
                surface.get(),
                capabilities.minImageCount + 1,
                vk::Format::eB8G8R8A8Unorm,
                vk::ColorSpaceKHR::eSrgbNonlinear,
                {
                        static_cast<uint32_t>(size.width),
                        static_cast<uint32_t>(size.height),
                },
                1,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
                vk::SharingMode::eExclusive,
                {},
                vk::SurfaceTransformFlagBitsKHR::eIdentity,
                vk::CompositeAlphaFlagBitsKHR::eOpaque,
                vk::PresentModeKHR::eMailbox,
                false,
        };
        if (swapchain) {
            swapchainCreateInfo.oldSwapchain = *swapchain;
        }
        swapchain = device->createSwapchainKHRUnique(swapchainCreateInfo);

        swapchainImages = device->getSwapchainImagesKHR(*swapchain);
    }
}  // namespace rendering
