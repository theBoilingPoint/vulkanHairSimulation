#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <unordered_map>

#include "vertex.h"
#include "renderPass.h"

class Pipeline {
private:
	VkDevice* device;

	VkShaderModule createShaderModule(const std::vector<char>& code);

public:
	RenderPass renderPass;
	VkPipelineLayout layout;
	VkPipeline pipeline;

	Pipeline() = delete;
	Pipeline(VkDevice* device, RenderPass renderPass);
	~Pipeline();

	void createPipelineLayout(VkDescriptorSetLayout* descriptorLayout);
	// This pipeline creation assumes dynamic viewport and scissor.
	void createPipeline(
		std::vector<char>vertShaderCode, 
		std::vector<char>fragShaderCode,
		bool useHardCodedVertices,
		VkPipelineRasterizationStateCreateInfo rasterizer,
		VkPipelineMultisampleStateCreateInfo multisampling,
		VkPipelineDepthStencilStateCreateInfo depthStencil,
		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments,
		uint32_t stageCount,
		uint32_t subpassIndex
	);

	void cleanUp();
};