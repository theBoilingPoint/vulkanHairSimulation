//#include "pipeline.h"
//
//Pipeline::Pipeline(
//	std::vector<ShaderStageInfo> shaderStages,
//	VertexInputInfo vertexInputInfo,
//	InputAssembly inputAssembly,
//	ViewportState viewportState,
//	Rasterizer rasterizer,
//	Multisampling multisampling,
//	DepthStencil depthStencil,
//	DynamicState dynamicState,
//	ColorBlending colorBlending) :
//	shaderStages(shaderStages),
//	vertexInputInfo(vertexInputInfo),
//	inputAssembly(inputAssembly),
//	viewportState(viewportState),
//	rasterizer(rasterizer),
//	multisampling(multisampling),
//	depthStencil(depthStencil),
//	dynamicState(dynamicState),
//	colorBlending(colorBlending) { };
//
//Pipeline::~Pipeline() {
//	for (auto& shaderStage : shaderStages) {
//		if (shaderStage.module != VK_NULL_HANDLE) {
//			vkDestroyShaderModule(*device, shaderStage.module, nullptr);
//		}
//	}
//	shaderStages.clear();
//
//	if (pipelineLayout != VK_NULL_HANDLE) {
//		vkDestroyPipelineLayout(*device, pipelineLayout, nullptr);
//	}
//	if (pipeline != VK_NULL_HANDLE) {
//		vkDestroyPipeline(*device, pipeline, nullptr);
//	}
//}
//
//void Pipeline::create(VkRenderPass renderPass, VkDevice *device, VkDescriptorSetLayout descriptorSetLayout) {
//	std::vector<VkPipelineShaderStageCreateInfo> shaderStagesCreateInfo;
//	for (auto& shaderStage : shaderStages) {
//		shaderStagesCreateInfo.push_back(shaderStage.create());
//	}
//
//	VkPipelineVertexInputStateCreateInfo vertexInputInfo = this->vertexInputInfo.create();
//	VkPipelineInputAssemblyStateCreateInfo inputAssembly = this->inputAssembly.create();
//	VkPipelineViewportStateCreateInfo viewportState = this->viewportState.create();
//	VkPipelineRasterizationStateCreateInfo rasterizer = this->rasterizer.create();
//	VkPipelineMultisampleStateCreateInfo multisampling = this->multisampling.create();
//	VkPipelineDepthStencilStateCreateInfo depthStencil = this->depthStencil.create();
//	VkPipelineColorBlendStateCreateInfo colorBlending = this->colorBlending.create();
//	VkPipelineDynamicStateCreateInfo dynamicState = this->dynamicState.create();
//
//	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//	pipelineLayoutInfo.setLayoutCount = 1;
//	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
//	pipelineLayoutInfo.pushConstantRangeCount = 0;
//	pipelineLayoutInfo.pPushConstantRanges = nullptr;
//
//	if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
//		throw std::runtime_error("failed to create pipeline layout!");
//	}
//
//	VkGraphicsPipelineCreateInfo pipelineInfo{};
//	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStagesCreateInfo.size());
//	pipelineInfo.pStages = shaderStagesCreateInfo.data();
//	pipelineInfo.pVertexInputState = &vertexInputInfo;
//	pipelineInfo.pInputAssemblyState = &inputAssembly;
//	pipelineInfo.pViewportState = &viewportState;
//	pipelineInfo.pRasterizationState = &rasterizer;
//	pipelineInfo.pMultisampleState = &multisampling;
//	pipelineInfo.pColorBlendState = &colorBlending;
//	pipelineInfo.pDynamicState = &dynamicState;
//	pipelineInfo.layout = pipelineLayout;
//	pipelineInfo.renderPass = renderPass;
//
//	this->device = device;
//	if (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
//		throw std::runtime_error("Failed to create graphics pipeline!");
//	}
//}