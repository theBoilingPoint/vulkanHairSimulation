#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <vector>

#include "descriptor.h"
#include "vulkanImage.h"

class UI {
private:
	VkDevice* device;

public:
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;
	VkDescriptorPool imguiPool;

	UI();
	UI(VkDevice* device);
	~UI();

	void createImguiRenderPass(const VkFormat format);

	void createImguiFramebuffers(
		const std::vector<VulkanImage> swapChainImageViews,
		const uint32_t width,
		const uint32_t height
	);

	void recordImguiRenderPass(
		VkCommandBuffer commandBuffer,
		uint32_t imageIndex,
		std::vector<VkImage> swapChainImages,
		VkImageLayout oldLayout,
		VkExtent2D extent
	);

	void init(
		GLFWwindow* window,
		VkInstance instance,
		VkPhysicalDevice physicalDevice,
		uint32_t queryFamilyIndex,
		VkQueue queue,
		uint32_t minImageCount,
		uint32_t imageCount
	);

	void destroy();

	ImGuiIO& getIO();

	void drawNewFrame();

	void updateWindows();
};