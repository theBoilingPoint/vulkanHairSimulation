#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include "utils.h"

struct DescriptorBinding {
	VkDescriptorSetLayoutBinding binding;
	std::vector<VkBuffer> buffers;
	VkImageView imageView;
	VkSampler sampler;

	DescriptorBinding(
		VkDescriptorSetLayoutBinding binding,
		const std::vector<VkBuffer>& buffers = {},
		VkImageView imageView = VK_NULL_HANDLE,
		VkSampler sampler = VK_NULL_HANDLE
	) : binding(binding), buffers(buffers), imageView(imageView), sampler(sampler) {}
};

class Descriptor {
private:
	VkDevice* device;
	uint32_t numUniformBuffers;
	uint32_t numTextureBuffers;
	uint32_t numInputBuffers;
	uint32_t totalNumBuffers;

	std::vector<DescriptorBinding> bindings;

	void createDescriptorSetLayout();
	void allocateDescriptorSets();
	void updateDescriptorSets();

public:
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;

	Descriptor();
	Descriptor(VkDevice *device, uint32_t numUniformBuffers, uint32_t numTextureBuffers, uint32_t numInputBuffers);
	~Descriptor();

	void create();
	void destroy();

	void createDescriptorPool(const uint32_t poolSize = 1000);

	void addDescriptorSetLayoutBinding(
		uint32_t binding, 
		VkDescriptorType type, 
		VkShaderStageFlags 
		shaderStageFlags, 
		std::vector<VkBuffer> buffers = {},
		VkImageView imageView = VK_NULL_HANDLE,
		VkSampler sampler = VK_NULL_HANDLE
	);
};