#include "descriptor.h"

Descriptor::Descriptor() : 
	device(nullptr),
	numUniformBuffers(0),
	numTextureBuffers(0),
	numInputBuffers(0),
	totalNumBuffers(0) {}

Descriptor::Descriptor(
	VkDevice* device, 
	uint32_t numUniformBuffers, 
	uint32_t numTextureBuffers, 
	uint32_t numInputBuffers
): 
	device(device), 
	numUniformBuffers(numUniformBuffers), 
	numTextureBuffers(numTextureBuffers), 
	numInputBuffers(numInputBuffers) {
	totalNumBuffers = numUniformBuffers + numTextureBuffers + numInputBuffers;
}

Descriptor::~Descriptor() {}

void Descriptor::create() {
	createDescriptorPool();
	createDescriptorSetLayout();
	allocateDescriptorSets();
	updateDescriptorSets();
}

void Descriptor::destroy() {
	if (descriptorPool == VK_NULL_HANDLE && descriptorSetLayout == VK_NULL_HANDLE) {
		return;
	}

	// No need to free individual sets ¨C destroying the pool does it.
	descriptorSets.clear();

	if (descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(*device, descriptorPool, nullptr);
		descriptorPool = VK_NULL_HANDLE;
	}

	if (descriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(*device, descriptorSetLayout, nullptr);
		descriptorSetLayout = VK_NULL_HANDLE;
	}

	bindings.clear();
}

void Descriptor::addDescriptorSetLayoutBinding(
	uint32_t binding, 
	VkDescriptorType type, 
	VkShaderStageFlags shaderStageFlags, 
	std::vector<VkBuffer> buffers,
	VkImageView imageView,
	VkSampler sampler) {
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = type;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = shaderStageFlags;
	layoutBinding.pImmutableSamplers = nullptr;

	DescriptorBinding descriptorBinding(layoutBinding, buffers, imageView, sampler);

	bindings.push_back(descriptorBinding);
}

void Descriptor::createDescriptorSetLayout() {
	if (bindings.size() != totalNumBuffers) {
		throw std::runtime_error("Descriptor set layout bindings do not match the total number of buffers. Are you sure you have set the number of each type of buffers correctly?");
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	for (const auto& binding : this->bindings) {
		bindings.push_back(binding.binding);
	}
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
	}
}

void Descriptor::createDescriptorPool(const uint32_t poolSize) {
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER,                poolSize },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolSize },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          poolSize },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          poolSize },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   poolSize },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   poolSize },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         poolSize },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         poolSize },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, poolSize },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, poolSize },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       poolSize }
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = static_cast<uint32_t>(poolSize);

	if (vkCreateDescriptorPool(*device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void Descriptor::allocateDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(*device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}
}

void Descriptor::updateDescriptorSets() {
	std::vector<VkWriteDescriptorSet> writes;
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkDescriptorImageInfo>  imageInfos;

	writes.reserve(MAX_FRAMES_IN_FLIGHT * bindings.size());
	bufferInfos.reserve(MAX_FRAMES_IN_FLIGHT * numUniformBuffers);
	imageInfos.reserve(MAX_FRAMES_IN_FLIGHT * (numTextureBuffers + numInputBuffers));

	for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame) {
		for (auto& b : bindings) {
			VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			w.dstSet = descriptorSets[frame];
			w.dstBinding = b.binding.binding;
			w.dstArrayElement = 0;
			w.descriptorType = b.binding.descriptorType;
			w.descriptorCount = 1;

			switch (b.binding.descriptorType) {
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
				bufferInfos.emplace_back();                         
				auto& info = bufferInfos.back();
				info.buffer = b.buffers[frame];
				info.offset = 0;
				info.range = sizeof(UniformBufferObject);
				w.pBufferInfo = &info;
				break;
			}
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
				imageInfos.emplace_back();
				auto& info = imageInfos.back();
				info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				info.imageView = b.imageView;
				info.sampler = b.sampler;
				w.pImageInfo = &info;
				break;
			}
			default:
				throw std::runtime_error("Unsupported descriptor type");
			}

			writes.push_back(w);
		}
	}

	vkUpdateDescriptorSets(*device,
		static_cast<uint32_t>(writes.size()), writes.data(),
		0, nullptr);
}