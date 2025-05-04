#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <stdexcept>
#include <cassert>
#include <vector>

#define ARRAY_SIZE(X) (sizeof((X))/sizeof((X)[0]))

// From NVIDIA (nvpro_core)
inline uint32_t makeAccessMaskPipelineStageFlags(uint32_t accessMask, VkPipelineStageFlags supportedShaderBits =
	VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
	| VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
	| VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
	| VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT
	| VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	| VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
	static const uint32_t accessPipes[] = {
	  VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
	  VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
	  VK_ACCESS_INDEX_READ_BIT,
	  VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
	  VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
	  VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
	  VK_ACCESS_UNIFORM_READ_BIT,
	  supportedShaderBits,
	  VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
	  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	  VK_ACCESS_SHADER_READ_BIT,
	  VK_ACCESS_SHADER_WRITE_BIT,
	  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
	  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	  VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT,
	  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
	  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
	  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
	  VK_ACCESS_TRANSFER_READ_BIT,
	  VK_PIPELINE_STAGE_TRANSFER_BIT,
	  VK_ACCESS_TRANSFER_WRITE_BIT,
	  VK_PIPELINE_STAGE_TRANSFER_BIT,
	  VK_ACCESS_HOST_READ_BIT,
	  VK_PIPELINE_STAGE_HOST_BIT,
	  VK_ACCESS_HOST_WRITE_BIT,
	  VK_PIPELINE_STAGE_HOST_BIT,
	  VK_ACCESS_MEMORY_READ_BIT,
	  0,
	  VK_ACCESS_MEMORY_WRITE_BIT,
	  0,
	};

	if (!accessMask)
	{
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}

	uint32_t pipes = 0;

	for (uint32_t i = 0; i < ARRAY_SIZE(accessPipes); i += 2)
	{
		if (accessPipes[i] & accessMask)
		{
			pipes |= accessPipes[i + 1];
		}
	}

	assert(pipes != 0);

	return pipes;
}

// From NVIDIA (nvpro_core)
inline VkImageMemoryBarrier makeImageMemoryBarrier(VkImage img, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask) {
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = srcAccess;
	barrier.dstAccessMask = dstAccess;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = img;
	barrier.subresourceRange = { 0 };
	barrier.subresourceRange.aspectMask = aspectMask;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	return barrier;
}

// From NVIDIA (nvpro_core)
inline void cmdImageTransition(VkCommandBuffer cmd, VkImage img, VkImageAspectFlags aspects, VkAccessFlags src, VkAccessFlags dst, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkPipelineStageFlags srcPipe = makeAccessMaskPipelineStageFlags(src);
	VkPipelineStageFlags dstPipe = makeAccessMaskPipelineStageFlags(dst);
	VkImageMemoryBarrier barrier = makeImageMemoryBarrier(img, src, dst, oldLayout, newLayout, aspects);

	vkCmdPipelineBarrier(cmd, srcPipe, dstPipe, VK_FALSE, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);
}

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
};