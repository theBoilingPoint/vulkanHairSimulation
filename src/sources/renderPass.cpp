#include "renderPass.h"

RenderPass::RenderPass() : device(nullptr), renderPass(VK_NULL_HANDLE) {}

RenderPass::RenderPass(VkDevice* device) : device(device), renderPass(VK_NULL_HANDLE) {}

RenderPass::~RenderPass() {}

void RenderPass::addAttachment(
	VkFormat format,
	VkSampleCountFlagBits samples,
	VkAttachmentLoadOp loadOp,
	VkAttachmentStoreOp storeOp,
	VkAttachmentLoadOp stencilLoadOp,
	VkAttachmentStoreOp stencilStoreOp,
	VkImageLayout initialLayout,
	VkImageLayout finalLayout
) {
	VkAttachmentDescription attachment = {};
	attachment.format = format;
	attachment.samples = samples;
	attachment.loadOp = loadOp;
	attachment.storeOp = storeOp;
	attachment.stencilLoadOp = stencilLoadOp;
	attachment.stencilStoreOp = stencilStoreOp;
	attachment.initialLayout = initialLayout;
	attachment.finalLayout = finalLayout;

	renderPassAttachments.emplace_back(attachment);
}

void RenderPass::addSubpass(
	VkPipelineBindPoint bindPoint,
	AttachmentUsageMap usageMap) {
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = bindPoint;

	subpassColorRefs.emplace_back();
	subpassInputRefs.emplace_back();
	subpassDepthRefs.emplace_back();
	for (auto& pair : usageMap) {
		switch (pair.second) {
			case COLOR: {
				VkAttachmentReference colorAttachment = {};
				colorAttachment.attachment = pair.first;
				colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				subpassColorRefs.back().push_back(colorAttachment);
				break;
			}
			case DEPTH: {
				VkAttachmentReference depthAttachment = {};
				depthAttachment.attachment = pair.first;
				depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				subpassDepthRefs.back().push_back(depthAttachment);
				break;
			}
			case INPUT: {
				VkAttachmentReference inputAttachment = {};
				inputAttachment.attachment = pair.first;
				inputAttachment.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				subpassInputRefs.back().push_back(inputAttachment);
				break;
			}
			default: {
				throw std::runtime_error("Unknown attachment type!");
			}
		}	
	}

	subpass.colorAttachmentCount = static_cast<uint32_t>(subpassColorRefs.back().size());
	subpass.pColorAttachments = subpassColorRefs.back().empty() ? nullptr : subpassColorRefs.back().data();
	subpass.inputAttachmentCount = static_cast<uint32_t>(subpassInputRefs.back().size());
	subpass.pInputAttachments = subpassInputRefs.back().empty() ? nullptr : subpassInputRefs.back().data();
	subpass.pDepthStencilAttachment = subpassDepthRefs.back().empty() ? nullptr : &subpassDepthRefs.back()[0];

	subpasses.emplace_back(subpass);
}

void RenderPass::createRenderPass(const std::vector<VkSubpassDependency>& dependencies) {
	subpassDependencies = dependencies;

	VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
	renderPassInfo.pAttachments = renderPassAttachments.data();
	renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	renderPassInfo.pSubpasses = subpasses.data();
	renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassInfo.pDependencies = subpassDependencies.empty() ? nullptr
		: subpassDependencies.data();

	if (vkCreateRenderPass(*device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create opaque objects render pass!");
	}
}

void  RenderPass::destroy() {
	if (renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(*device, renderPass, nullptr);
		renderPass = VK_NULL_HANDLE;
	}
	renderPassAttachments.clear();
	subpasses.clear();
	subpassDependencies.clear();
}