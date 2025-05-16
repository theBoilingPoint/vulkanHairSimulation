#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <vector>

#include "descriptor.h"
#include "vulkanImage.h"
#include "utils.h"

class UI {
private:
	VkDevice* device;

public:
	VkDescriptorPool imguiPool;

	UI();
	UI(VkDevice* device);
	~UI();

	void init(
		GLFWwindow* window,
		VkInstance instance,
		VkPhysicalDevice physicalDevice,
		uint32_t queryFamilyIndex,
		VkQueue queue,
		VkRenderPass renderPass,
		uint32_t minImageCount,
		uint32_t imageCount
	);

	void destroy();

	ImGuiIO& getIO();

	void drawNewFrame(UIState &state);

	void updateWindows();
};