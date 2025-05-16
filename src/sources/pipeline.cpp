#include "pipeline.h"

Pipeline::Pipeline() : device(nullptr), renderPass() {}

Pipeline::Pipeline(VkDevice* device, RenderPass renderPass)
	: device(device), renderPass(renderPass) {}

Pipeline::~Pipeline() {}

void Pipeline::destroy() {
	if (layout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(*device, layout, nullptr);
		layout = VK_NULL_HANDLE;
	}

	if (pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(*device, pipeline, nullptr);
		pipeline = VK_NULL_HANDLE;
	}
}

void Pipeline::createPipelineLayout(VkDescriptorSetLayout *descriptorLayout) {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = descriptorLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

VkShaderModule Pipeline::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	// the size of the bytecode is specified in bytes, but the bytecode pointer is a uint32_t pointer rather than a char pointer. 
	// Therefore we will need to cast the pointer with reinterpret_cast as shown below.
	// We also need to ensure that the data satisfies the alignment requirements of uint32_t. 
	// Lucky for us, the data is stored in an std::vector where the default allocator already ensures that the data satisfies 
	// the worst case alignment requirements.
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(*device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	return shaderModule;
}

void Pipeline::createPipeline(
	std::vector<char>vertShaderCode, 
	std::vector<char>fragShaderCode,
	bool useHardCodedVertices,
	VkPipelineRasterizationStateCreateInfo rasterizer,
	VkSampleCountFlagBits msaaSamples,
	VkPipelineDepthStencilStateCreateInfo depthStencil,
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments,
	uint32_t subpassIndex) {
	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	// keep the storage alive for the whole function
	std::vector<VkVertexInputBindingDescription>   bindingDescription;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

	if (useHardCodedVertices) {
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;
	}
	else {
		bindingDescription = Vertex::getBindingDescription();
		attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount =
			static_cast<uint32_t>(bindingDescription.size());
		vertexInputInfo.vertexAttributeDescriptionCount =
			static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(std::size(dynamicStates));
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
	multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother

	const VkColorComponentFlags colorFlags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = colorBlendAttachments.size();
	colorBlending.pAttachments = colorBlendAttachments.data();
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(std::size(shaderStages));
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.layout = layout;
	pipelineInfo.renderPass = renderPass.renderPass;
	pipelineInfo.subpass = subpassIndex;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	
	if (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create opaque objects pipeline!");
	}

	vkDestroyShaderModule(*device, fragShaderModule, nullptr);
	vkDestroyShaderModule(*device, vertShaderModule, nullptr);
}