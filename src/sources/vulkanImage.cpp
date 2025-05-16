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
	currentAccesses(0),
	createFlags(0),
	viewType(VK_IMAGE_VIEW_TYPE_2D) { }

VulkanImage::VulkanImage(
	VkDevice* device,
	uint32_t width,
	uint32_t height,
	uint32_t mipLevels,
	VkSampleCountFlagBits samples,
	VkFormat format,
	VkImageUsageFlags usage,
	VkImageAspectFlags aspectFlags,
	VkImageTiling tiling,
	uint32_t layers,
	VkImageCreateFlags createFlags,
	VkImageViewType viewType
) :
	device(device),
	width(width),
	height(height),
	mipLevels(mipLevels),
	samples(samples),
	format(format),
	usage(usage),
	aspectFlags(aspectFlags),
	image(nullptr),
	view(nullptr),
	memory(nullptr),
	currentLayout(VK_IMAGE_LAYOUT_UNDEFINED),
	currentAccesses(0),
	tiling(tiling),
	layers(layers),
	createFlags(createFlags),
	viewType(viewType) {}

VulkanImage::VulkanImage(
	VkDevice* device, 
	VkImage swapChainImage, 
	VkFormat swapchainFormat, 
	VkImageAspectFlags swapchainAspectFlags, 
	uint32_t swapchainMipLevels
):
	device(device),
	width(0),
	height(0),
	mipLevels(swapchainMipLevels),
	layers(1),
	tiling(VK_IMAGE_TILING_OPTIMAL),
	usage(0),
	aspectFlags(swapchainAspectFlags),
	samples(VK_SAMPLE_COUNT_1_BIT),
	format(swapchainFormat),
	image(swapChainImage),
	view(nullptr),
	memory(nullptr),
	currentLayout(VK_IMAGE_LAYOUT_UNDEFINED),
	currentAccesses(0),
	createFlags(0),
	viewType(VK_IMAGE_VIEW_TYPE_2D) {}

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
	imageInfo.flags = createFlags;

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
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = layers;

	if (vkCreateImageView(*device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}
}

void VulkanImage::transitionLayout(
	VkCommandBuffer commandBuffer, 
	VkImageLayout newLayout, 
	VkAccessFlags newtAccesses,
	VkImageSubresourceRange layers
) {
	VkImageAspectFlags aspectMask = 0;
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
		{
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	cmdImageTransition(commandBuffer, image, aspectMask, currentAccesses, newtAccesses, currentLayout, newLayout, layers);

	// Update current layout, stages, and accesses
	currentLayout = newLayout;
	currentAccesses = newtAccesses;
}