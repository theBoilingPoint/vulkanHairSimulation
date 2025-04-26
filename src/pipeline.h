#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <array>

struct ShaderStageInfo {
	VkShaderStageFlagBits stage;
	VkShaderModule module;
	std::string pName;

	ShaderStageInfo() {}
	ShaderStageInfo(
		VkShaderStageFlagBits stage, 
		VkShaderModule module, 
		std::string pName) :
		stage(stage), 
		module(module), 
		pName(pName) { }

	VkPipelineShaderStageCreateInfo create() {
		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = stage;
		shaderStageInfo.module = module;
		shaderStageInfo.pName = pName.c_str();
		return shaderStageInfo;
	};
};

struct VertexInputInfo {
	uint32_t vertexBindingDescriptionCount;
	uint32_t vertexAttributeDescriptionCount;
	VkVertexInputBindingDescription	vertexBindingDescriptions;
	VkVertexInputAttributeDescription vertexAttributeDescriptions;

	VertexInputInfo() {}
	VertexInputInfo(
		uint32_t vertexBindingDescriptionCount,
		uint32_t vertexAttributeDescriptionCount,
		VkVertexInputBindingDescription	vertexBindingDescriptions,
		VkVertexInputAttributeDescription vertexAttributeDescriptions) :
		vertexBindingDescriptionCount(vertexBindingDescriptionCount),
		vertexAttributeDescriptionCount(vertexAttributeDescriptionCount),
		vertexBindingDescriptions(vertexBindingDescriptions),
		vertexAttributeDescriptions(vertexAttributeDescriptions) { }

	VkPipelineVertexInputStateCreateInfo create() {
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = vertexBindingDescriptionCount;
		vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
		vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescriptions;
		vertexInputInfo.pVertexAttributeDescriptions = &vertexAttributeDescriptions;
		return vertexInputInfo;
	}
};

struct InputAssembly {
	VkPrimitiveTopology topology;
	VkBool32 primitiveRestartEnable;

	InputAssembly() {}
	InputAssembly(
		VkPrimitiveTopology topology,
		VkBool32 primitiveRestartEnable) :
		topology(topology),
		primitiveRestartEnable(primitiveRestartEnable) { }

	VkPipelineInputAssemblyStateCreateInfo create() {
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = topology;
		inputAssembly.primitiveRestartEnable = primitiveRestartEnable;
		return inputAssembly;
	}
};

// Here I am assuming dynamic viewports
struct ViewportState {
	VkViewport viewport;
	VkRect2D scissor;

	ViewportState() {}
	ViewportState(
		VkViewport viewport,
		VkRect2D scissor) :
		viewport(viewport),
		scissor(scissor) {
	}

	VkPipelineViewportStateCreateInfo create() {
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;
		return viewportState;
	}
};

struct Rasterizer {
	VkPolygonMode polygonMode;
	VkCullModeFlags cullMode;
	VkFrontFace frontFace;
	VkBool32 depthClampEnable;
	VkBool32 rasterizerDiscardEnable;
	float lineWidth;

	Rasterizer() {}
	Rasterizer(
		VkPolygonMode polygonMode,
		VkCullModeFlags cullMode,
		VkFrontFace frontFace,
		VkBool32 depthClampEnable,
		VkBool32 rasterizerDiscardEnable,
		float lineWidth) :
		polygonMode(polygonMode),
		cullMode(cullMode),
		frontFace(frontFace),
		depthClampEnable(depthClampEnable),
		rasterizerDiscardEnable(rasterizerDiscardEnable),
		lineWidth(lineWidth) {}

	VkPipelineRasterizationStateCreateInfo create() {
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = depthClampEnable;
		rasterizer.rasterizerDiscardEnable = rasterizerDiscardEnable;
		rasterizer.polygonMode = polygonMode;
		rasterizer.lineWidth = lineWidth;
		rasterizer.cullMode = cullMode;
		rasterizer.frontFace = frontFace;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;
		return rasterizer;
	}
};

struct Multisampling {
	VkSampleCountFlagBits rasterizationSamples;
	VkBool32 sampleShadingEnable;
	float minSampleShading;
	VkSampleMask* pSampleMask;
	VkBool32 alphaToCoverageEnable;
	VkBool32 alphaToOneEnable;

	Multisampling() {}
	Multisampling(
		VkSampleCountFlagBits rasterizationSamples,
		VkBool32 sampleShadingEnable,
		float minSampleShading,
		VkSampleMask* pSampleMask,
		VkBool32 alphaToCoverageEnable,
		VkBool32 alphaToOneEnable) :
		rasterizationSamples(rasterizationSamples),
		sampleShadingEnable(sampleShadingEnable),
		minSampleShading(minSampleShading),
		pSampleMask(pSampleMask),
		alphaToCoverageEnable(alphaToCoverageEnable),
		alphaToOneEnable(alphaToOneEnable) {}

	VkPipelineMultisampleStateCreateInfo create() {
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = sampleShadingEnable;
		multisampling.rasterizationSamples = rasterizationSamples;
		multisampling.minSampleShading = minSampleShading;
		multisampling.pSampleMask = pSampleMask;
		multisampling.alphaToCoverageEnable = alphaToCoverageEnable;
		multisampling.alphaToOneEnable = alphaToOneEnable;
		return multisampling;
	}
};

struct DepthStencil {
	VkBool32 depthTestEnable;
	VkBool32 depthWriteEnable;
	VkCompareOp depthCompareOp;
	float minDepthBounds;
	float maxDepthBounds;
	VkBool32 stencilTestEnable;
	VkStencilOpState front;
	VkStencilOpState back;

	DepthStencil() {}
	DepthStencil(
		VkBool32 depthTestEnable,
		VkBool32 depthWriteEnable,
		VkCompareOp depthCompareOp,
		float minDepthBounds,
		float maxDepthBounds,
		VkBool32 stencilTestEnable,
		VkStencilOpState front,
		VkStencilOpState back) :
		depthTestEnable(depthTestEnable),
		depthWriteEnable(depthWriteEnable),
		depthCompareOp(depthCompareOp),
		minDepthBounds(minDepthBounds),
		maxDepthBounds(maxDepthBounds),
		stencilTestEnable(stencilTestEnable),
		front(front),
		back(back) { }

	VkPipelineDepthStencilStateCreateInfo create() {
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = depthTestEnable;
		depthStencil.depthWriteEnable = depthWriteEnable;
		depthStencil.depthCompareOp = depthCompareOp;
		depthStencil.minDepthBounds = minDepthBounds;
		depthStencil.maxDepthBounds = maxDepthBounds;
		depthStencil.stencilTestEnable = stencilTestEnable;
		depthStencil.front = front;
		depthStencil.back = back;
		return depthStencil;
	}
};

struct DynamicState {
	std::vector<VkDynamicState> dynamicStates;
	DynamicState() {}
	DynamicState(
		std::vector<VkDynamicState> dynamicStates) :
		dynamicStates(dynamicStates) {
	}
	VkPipelineDynamicStateCreateInfo create() {
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();
		return dynamicState;
	}
};

struct ColorBlendAttachment {
	VkColorComponentFlags colorWriteMask;
	VkBool32 blendEnable;
	VkBlendFactor srcColorBlendFactor;
	VkBlendFactor dstColorBlendFactor;
	VkBlendOp colorBlendOp;
	VkBlendFactor srcAlphaBlendFactor;
	VkBlendFactor dstAlphaBlendFactor;
	VkBlendOp alphaBlendOp;

	ColorBlendAttachment() {}
	ColorBlendAttachment(
		VkColorComponentFlags colorWriteMask,
		VkBool32 blendEnable,
		VkBlendFactor srcColorBlendFactor,
		VkBlendFactor dstColorBlendFactor,
		VkBlendOp colorBlendOp,
		VkBlendFactor srcAlphaBlendFactor,
		VkBlendFactor dstAlphaBlendFactor,
		VkBlendOp alphaBlendOp) :
		colorWriteMask(colorWriteMask),
		blendEnable(blendEnable),
		srcColorBlendFactor(srcColorBlendFactor),
		dstColorBlendFactor(dstColorBlendFactor),
		colorBlendOp(colorBlendOp),
		srcAlphaBlendFactor(srcAlphaBlendFactor),
		dstAlphaBlendFactor(dstAlphaBlendFactor),
		alphaBlendOp(alphaBlendOp) {}

	VkPipelineColorBlendAttachmentState create() {
		VkPipelineColorBlendAttachmentState colorAttachment{};
		colorAttachment.colorWriteMask = colorWriteMask;
		colorAttachment.blendEnable = blendEnable;
		colorAttachment.srcColorBlendFactor = srcColorBlendFactor;
		colorAttachment.dstColorBlendFactor = dstColorBlendFactor;
		colorAttachment.colorBlendOp = colorBlendOp;
		colorAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
		colorAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
		colorAttachment.alphaBlendOp = alphaBlendOp;
		return colorAttachment;
	}
};

struct ColorBlending {
	VkBool32 logicOpEnable;
	VkLogicOp logicOp;
	std::vector<VkPipelineColorBlendAttachmentState> pAttachments;

	ColorBlending() { }
	ColorBlending(
		VkBool32 logicOpEnable,
		VkLogicOp logicOp,
		std::vector<VkPipelineColorBlendAttachmentState> pAttachments)
		: logicOpEnable(logicOpEnable),
		logicOp(logicOp),
		pAttachments(pAttachments) { }

	VkPipelineColorBlendStateCreateInfo create() {
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = logicOpEnable;
		colorBlending.logicOp = logicOp;
		colorBlending.attachmentCount = pAttachments.size();
		colorBlending.pAttachments = pAttachments.data();
		for (size_t i = 0; i < colorBlending.attachmentCount; i++) {
			colorBlending.blendConstants[i] = 0.0f;
		}
		return colorBlending;
	}
};

struct PipelineLayoutInfo {
	uint32_t descriptorSetLayoutCount;
	VkDescriptorSetLayout descriptorSetLayout;
	uint32_t pushConstantRangeCount;
	VkPushConstantRange* pPushConstantRanges;
	PipelineLayoutInfo() {}
	PipelineLayoutInfo(
		uint32_t descriptorSetLayoutCount,
		VkDescriptorSetLayout descriptorSetLayout,
		uint32_t pushConstantRangeCount,
		VkPushConstantRange* pPushConstantRanges) :
		descriptorSetLayoutCount(descriptorSetLayoutCount),
		descriptorSetLayout(descriptorSetLayout),
		pushConstantRangeCount(pushConstantRangeCount),
		pPushConstantRanges(pPushConstantRanges) {
	}
	VkPipelineLayoutCreateInfo create() {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = descriptorSetLayoutCount;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRangeCount;
		pipelineLayoutInfo.pPushConstantRanges = pPushConstantRanges;
		return pipelineLayoutInfo;
	}
};

class Pipeline {
private:
	VkDevice* device;

public:
	std::vector<ShaderStageInfo> shaderStages;
	VertexInputInfo vertexInputInfo;
	InputAssembly inputAssembly;
	ViewportState viewportState;
	Rasterizer rasterizer;
	Multisampling multisampling;
	DepthStencil depthStencil;
	DynamicState dynamicState;
	ColorBlending colorBlending;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	Pipeline() {}

	Pipeline(
		std::vector<ShaderStageInfo> shaderStages,
		VertexInputInfo vertexInputInfo,
		InputAssembly inputAssembly,
		ViewportState viewportState,
		Rasterizer rasterizer,
		Multisampling multisampling,
		DepthStencil depthStencil,
		DynamicState dynamicState,
		ColorBlending colorBlending) { };

	~Pipeline();

	void create(VkRenderPass renderPass, VkDevice *device, VkDescriptorSetLayout descriptorSetLayout);
	void cleanUp();

};