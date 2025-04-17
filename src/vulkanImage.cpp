#include "vulkanImage.h"

VulkanImage::VulkanImage() :
	device(nullptr),
	width(0),
	height(0),
	mipLevels(1),
	layers(1),
	tiling(VK_IMAGE_TILING_OPTIMAL),
	usage(0),
	aspectFlags(0),
	samples(VK_SAMPLE_COUNT_1_BIT),
	format(VK_FORMAT_UNDEFINED),
	image(nullptr),
	view(nullptr),
	memory(nullptr),
	currentLayout(VK_IMAGE_LAYOUT_UNDEFINED),
	currentAccesses(0) {}

VulkanImage::VulkanImage(VkDevice* device, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags) {
	this->device = device;
	this->width = width;
	this->height = height;
	this->mipLevels = mipLevels;
	this->samples = samples;
	this->format = format;
	this->tiling = tiling;
	this->usage = usage;
	this->aspectFlags = aspectFlags;

	currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	currentAccesses = 0;
	layers = 1;
	image = nullptr;
	view = nullptr;
	memory = nullptr;
}

VulkanImage::VulkanImage(VkDevice* device, VkImage swapChainImage, VkFormat swapchainFormat, VkImageAspectFlags swapchainAspectFlags, uint32_t swapchainMipLevels) {
	this->device = device;
	this->image = swapChainImage;
	this->format = swapchainFormat;
	this->aspectFlags = swapchainAspectFlags;
	this->mipLevels = swapchainMipLevels;

	width = -1;
	height = -1;
	currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	currentAccesses = 0;
	layers = 1;
	view = nullptr;
	memory = nullptr;
	samples = VK_SAMPLE_COUNT_1_BIT;
	tiling = VK_IMAGE_TILING_OPTIMAL;
	usage = 0;
}

VulkanImage::~VulkanImage() {}

void VulkanImage::destroy() {
	vkDestroyImageView(*device, view, nullptr);
	vkDestroyImage(*device, image, nullptr);
	vkFreeMemory(*device, memory, nullptr);
}

void VulkanImage::destroySwapchainView() {
	vkDestroyImageView(*device, view, nullptr);
}

void VulkanImage::createImage() {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = layers;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = samples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(*device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}
}

void VulkanImage::bindMemory(const VkDeviceSize allocationSize, const uint32_t memoryTypeIndex) {
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = allocationSize;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(*device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate image memory!");
	}

	vkBindImageMemory(*device, image, memory, 0);
}

void VulkanImage::createView() {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(*device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}
}

void VulkanImage::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout) {
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = currentLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (currentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
	
	currentLayout = newLayout;
	currentAccesses = barrier.dstAccessMask;
}