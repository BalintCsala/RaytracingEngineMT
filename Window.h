#pragma once

#include "glfw_include.h"
#include <string>
#include <vulkan/vulkan.hpp>

namespace rendering
{

	class Window
	{
	public:
		GLFWwindow* handle;

		explicit Window(const std::string &title);
		~Window();
		Window(Window& other) = delete;
		Window(Window&& other) noexcept;
		Window& operator=(Window& other) = delete;
		Window& operator=(Window&& other) noexcept;

		[[nodiscard]] vk::Extent2D getSize() const;

		[[nodiscard]] bool update() const;
	};

}