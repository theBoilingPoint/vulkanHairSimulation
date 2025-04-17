#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <stdexcept>
#include <cassert>
#include <vector>

#define ARRAY_SIZE(X) (sizeof((X))/sizeof((X)[0]))

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
	// From NVIDIA (nvpro_core)
	void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, VkAccessFlags newtAccesses);

private:
	VkDevice *device;
	VkDeviceMemory memory;
	uint32_t mipLevels;
	VkImageTiling tiling;
	VkImageUsageFlags usage;
	VkImageAspectFlags aspectFlags;
	VkSampleCountFlagBits samples;

	// From NVIDIA (nvpro_core)
	void cmdImageTransition(VkCommandBuffer cmd, VkImage img, VkImageAspectFlags aspects, VkAccessFlags src, VkAccessFlags dst, VkImageLayout oldLayout, VkImageLayout newLayout);
	// From NVIDIA (nvpro_core)
	uint32_t makeAccessMaskPipelineStageFlags(uint32_t accessMask,
		VkPipelineStageFlags supportedShaderBits = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
		| VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
		| VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		| VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	// From NVIDIA (nvpro_core)
	VkImageMemoryBarrier makeImageMemoryBarrier(VkImage img, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask);
};