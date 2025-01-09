#include "Window.h"

namespace rendering {

Window::Window(const std::string &title) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    handle = glfwCreateWindow(1920, 1080, title.c_str(), nullptr, nullptr);
    glfwShowWindow(handle);
}

Window::~Window() {
    glfwDestroyWindow(handle);
    glfwTerminate();
}

Window::Window(Window &&other) noexcept {
    handle = other.handle;
}

Window &Window::operator=(Window &&other) noexcept {
    handle = other.handle;
    return *this;
}

vk::Extent2D Window::getSize() const {
    int32_t width, height;
    glfwGetWindowSize(handle, &width, &height);
    return vk::Extent2D{
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
    };
}

bool Window::update() const {
    glfwPollEvents();
    return !glfwWindowShouldClose(handle);
}

}  // namespace rendering
