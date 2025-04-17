#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

class VulkanImage {
public:
	uint32_t width;
	uint32_t height;
	uint32_t layers;
	VkFormat format;
	VkImageLayout currentLayout;
	VkAccessFlags currentAccesses;
	VkImage image;
	VkImageView view;

	VulkanImage();
	VulkanImage(VkDevice* device, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags);
	VulkanImage(VkDevice* device, VkImage swapChainImage, VkFormat swapchainFormat, VkImageAspectFlags swapchainAspectFlags, uint32_t swapchainMipLevels);
	
	~VulkanImage();

	void destroy();
	void destroySwapchainView();
	void createImage();
	void bindMemory(const VkDeviceSize allocationSize, const uint32_t memoryTypeIndex);
	void createView();
	void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout);

private:
	VkDevice *device;
	VkDeviceMemory memory;
	uint32_t mipLevels;
	VkImageTiling tiling;
	VkImageUsageFlags usage;
	VkImageAspectFlags aspectFlags;
	VkSampleCountFlagBits samples;
};