#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <unordered_map>

#include <vector>

enum AttachmentType {
	COLOR,
	DEPTH,
	INPUT
};

// The first argument is the index of the attachment in renderPassAttachments,
// and the second argument is the type of attachment.
typedef std::vector<std::pair<uint32_t, AttachmentType>> AttachmentUsageMap;

class RenderPass {
private:
	VkDevice* device;
	std::vector<VkAttachmentDescription> renderPassAttachments;
	std::vector<std::vector<VkAttachmentReference>> subpassColorRefs;
	std::vector<std::vector<VkAttachmentReference>> subpassInputRefs;
	std::vector<std::vector<VkAttachmentReference>> subpassDepthRefs;
	std::vector<VkSubpassDescription> subpasses;
	std::vector<VkSubpassDependency> subpassDependencies;

public:
	VkRenderPass renderPass;

	RenderPass();
	RenderPass(VkDevice* device);
	~RenderPass();

	void addAttachment(
		VkFormat format,
		VkSampleCountFlagBits samples,
		VkAttachmentLoadOp loadOp,
		VkAttachmentStoreOp storeOp,
		VkAttachmentLoadOp stencilLoadOp,
		VkAttachmentStoreOp stencilStoreOp,
		VkImageLayout initialLayout,
		VkImageLayout finalLayout
	);

	void addSubpass(
		VkPipelineBindPoint bindPoint,
		AttachmentUsageMap usageMap
	);

	void createRenderPass(const std::vector<VkSubpassDependency>& deps);

	void destroy();
};