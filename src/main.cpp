#define GLFW_INCLUDE_VULKAN

#include "VulkanContext.h"

#include <iostream>
#include <string>
#include <vulkan/vulkan.hpp>
#include <fstream>
#include <filesystem>

#include "ComputePass.h"
#include "DescriptorSetAllocator.h"
#include "GLFW/glfw3.h"
#include "ProcessingPipeline.h"
#include "Scene.h"
#include "Window.h"

#define GLM_ENABLE_EXPERIMENTAL

#include "glm/gtx/euler_angles.hpp"

#include "tiny_gltf.h"
#include "stb_image.h"
#include "stb_image_write.h"


struct Uniforms {
    glm::mat4 proj;
    glm::mat4 projInverse;
    glm::mat4 view;
    glm::mat4 viewInverse;
    uint32_t frame;
};

float cameraSpeed = 3.0f;

void scrollCallback(GLFWwindow *window, double xoff, double yoff) {
    cameraSpeed *= std::pow(2.0f, static_cast<float>(yoff) / 50.0f);
}

int main() {
    rendering::Window window("Diplomaterv RT");
    rendering::VulkanContext context(window);
    std::filesystem::current_path("../");

    std::vector<rendering::PoolSizeInfo> poolSizeInfos;
    poolSizeInfos.push_back({.type = vk::DescriptorType::eStorageImage, .ratio = 0.3f});
    poolSizeInfos.push_back({.type = vk::DescriptorType::eStorageBuffer, .ratio = 0.3f});
    poolSizeInfos.push_back({.type = vk::DescriptorType::eUniformBuffer, .ratio = 0.2f});
    poolSizeInfos.push_back({.type = vk::DescriptorType::eCombinedImageSampler, .ratio = 0.1f});
    poolSizeInfos.push_back({.type = vk::DescriptorType::eAccelerationStructureKHR, .ratio = 0.1f});
    rendering::DescriptorSetAllocator descriptorSetAllocator(context, 1024, poolSizeInfos);

    const auto scene = std::make_shared<rendering::Scene>();
    std::ifstream sceneDescriptor("models/scene.txt");
    for (std::string line; std::getline(sceneDescriptor, line);) {
        scene->loadGLTF(context, "models/" + line, 0);
    }

    scene->build(context);

    rendering::Buffer uniforms(context, sizeof(Uniforms), vk::BufferUsageFlagBits::eUniformBuffer);
    rendering::Buffer uniformsSwap(context, sizeof(Uniforms), vk::BufferUsageFlagBits::eUniformBuffer);

    rendering::ProcessingPipeline processingPipeline("models/pipeline.json", scene);
    processingPipeline.build(context, descriptorSetAllocator, window.getSize());

    glm::vec3 position(0.0f, 0.5f, -2.0f);
    float yaw = 0.0f;
    float pitch = 0.0f;
    float prevTime = 0.0f;
    const float angularSpeed = 0.005f;
    double prevMouseX, prevMouseY;

    glfwSetScrollCallback(window.handle, scrollCallback);

    uint32_t frameIndex = 0;
    bool reloaded = false;

    auto start = std::chrono::high_resolution_clock::now();
    auto countedFrames = 0;

    while (window.update()) {
        // Inputs
        int reloadKeyState = glfwGetKey(window.handle, GLFW_KEY_R) == GLFW_PRESS;
        if (reloadKeyState == GLFW_PRESS) {
            if (!reloaded) {
                processingPipeline.build(context, descriptorSetAllocator, window.getSize());
                start = std::chrono::high_resolution_clock::now();
                countedFrames = 0;

                reloaded = true;
            }
        } else {
            reloaded = false;
        }

        auto currTime = static_cast<float>(glfwGetTime());
        auto dt = currTime - prevTime;
        prevTime = currTime;

        bool dragging = glfwGetMouseButton(window.handle, 0) != GLFW_RELEASE;
        double mouseX, mouseY;
        glfwGetCursorPos(window.handle, &mouseX, &mouseY);
        if (dragging) {
            yaw -= static_cast<float>(mouseX - prevMouseX) * angularSpeed;
            pitch -= static_cast<float>(mouseY - prevMouseY) * angularSpeed;
        }
        prevMouseX = mouseX;
        prevMouseY = mouseY;

        glm::mat4 movementRotation = glm::eulerAngleY(yaw);
        glm::mat4 yawRotation = glm::eulerAngleY(-yaw);
        glm::mat4 pitchRotation = glm::eulerAngleX(-pitch);
        glm::vec3 xAxis = glm::vec3(movementRotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
        glm::vec3 zAxis = glm::vec3(movementRotation * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
        if (glfwGetKey(window.handle, GLFW_KEY_W) != GLFW_RELEASE) {
            position -= zAxis * dt * cameraSpeed;
        }
        if (glfwGetKey(window.handle, GLFW_KEY_S) != GLFW_RELEASE) {
            position += zAxis * dt * cameraSpeed;
        }
        if (glfwGetKey(window.handle, GLFW_KEY_D) != GLFW_RELEASE) {
            position += xAxis * dt * cameraSpeed;
        }
        if (glfwGetKey(window.handle, GLFW_KEY_A) != GLFW_RELEASE) {
            position -= xAxis * dt * cameraSpeed;
        }
        if (glfwGetKey(window.handle, GLFW_KEY_SPACE) != GLFW_RELEASE) {
            position.y += dt * cameraSpeed;
        }
        if (glfwGetKey(window.handle, GLFW_KEY_LEFT_SHIFT) != GLFW_RELEASE) {
            position.y -= dt * cameraSpeed;
        }
        const auto fovy = glm::radians(90.0f);
        const auto aspect = static_cast<float>(window.getSize().width) / static_cast<float>(window.getSize().height);
        glm::mat4 proj = glm::perspective(fovy, aspect, 0.01f, 100.0f);
        glm::mat4 projInverse = glm::inverse(proj);
        glm::mat4 view = pitchRotation * yawRotation * glm::translate(glm::mat4(1.0f), -position);
        glm::mat4 viewInverse = glm::inverse(view);
        Uniforms uniformData{
                proj,
                projInverse,
                view,
                viewInverse,
                frameIndex,
        };
        if (frameIndex % 2 == 0) {
            uniforms.updateData(context, sizeof(Uniforms), &uniformData);
            processingPipeline.updateUniforms<Uniforms>(context, *uniforms.buffer, *uniformsSwap.buffer);
        } else {
            uniformsSwap.updateData(context, sizeof(Uniforms), &uniformData);
            processingPipeline.updateUniforms<Uniforms>(context, *uniformsSwap.buffer, *uniforms.buffer);
        }

        const auto &frame = context.getFrame(frameIndex);
        const auto result = context.device->waitForFences({*frame.renderFence}, true, 10000000000);
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to wait for rendering fence");
        }

        uint32_t swapchainIndex;
        try {
            swapchainIndex =
                    context.device->acquireNextImageKHR(
                            *context.swapchain,
                            1000000000,
                            *frame.swapchainSemaphore
                    ).value;
        } catch (vk::Error &err) {
            context.recreateSwapchain(window);
            std::cout << window.getSize().width << " " << window.getSize().height << "\n";
            processingPipeline.build(context, descriptorSetAllocator, window.getSize());
            continue;
        }
        context.device->resetFences({*frame.renderFence});

        frame.commandBuffer->reset();
        frame.commandBuffer->begin(
                {
                        vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
                }
        );

        processingPipeline.dispatch(*frame.commandBuffer);

        rendering::VulkanContext::transitionImage(
                *frame.commandBuffer,
                *processingPipeline.outputImage->image,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eTransferSrcOptimal
        );
        rendering::VulkanContext::transitionImage(
                *frame.commandBuffer,
                context.swapchainImages[swapchainIndex],
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal
        );

        const vk::ImageCopy imageCopy(
                {
                        vk::ImageAspectFlagBits::eColor,
                        0,
                        0,
                        1,
                },
                {},
                {
                        vk::ImageAspectFlagBits::eColor,
                        0,
                        0,
                        1,
                },
                {},
                {
                        processingPipeline.outputImage->size.width,
                        processingPipeline.outputImage->size.height,
                        1
                }
        );
        frame.commandBuffer->copyImage(
                *processingPipeline.outputImage->image,
                vk::ImageLayout::eTransferSrcOptimal,
                context.swapchainImages[swapchainIndex],
                vk::ImageLayout::eTransferDstOptimal,
                imageCopy
        );

        rendering::VulkanContext::transitionImage(
                *frame.commandBuffer,
                *processingPipeline.outputImage->image,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eGeneral
        );
        rendering::VulkanContext::transitionImage(
                *frame.commandBuffer,
                context.swapchainImages[swapchainIndex],
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::ePresentSrcKHR
        );

        frame.commandBuffer->end();

        const vk::SemaphoreSubmitInfo waitInfo{
                *frame.swapchainSemaphore,
                1,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                0,
        };
        const vk::SemaphoreSubmitInfo signalInfo{
                *frame.renderSemaphore,
                1,
                vk::PipelineStageFlagBits2::eAllGraphics,
                0,
        };
        const vk::CommandBufferSubmitInfo commandBufferSubmitInfo{
                *frame.commandBuffer,
                0,
        };

        const vk::SubmitInfo2 submitInfo{
                {},
                waitInfo,
                commandBufferSubmitInfo,
                signalInfo,
        };

        context.queue.submit2({submitInfo}, *frame.renderFence);
        context.queue.waitIdle();

        vk::PresentInfoKHR presentInfo{
                *frame.renderSemaphore,
                *context.swapchain,
                swapchainIndex
        };
        try {
            const auto presentResult = context.queue.presentKHR(presentInfo);
            if (presentResult != vk::Result::eSuccess) {
                std::cout << "Failed to present\n";
            }
        } catch (vk::OutOfDateKHRError &ex) {
        }

        frameIndex++;
        countedFrames++;
        if (countedFrames % 100 == 0) {
            std::cout << "FPS: " << (static_cast<double>(countedFrames) / static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count()) * 1000.0) << "\n";
        }
    }

    context.queue.waitIdle();

    return 0;
}
