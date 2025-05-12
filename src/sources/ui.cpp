#include "ui.h"

UI::UI() :
	renderPass(VK_NULL_HANDLE) {}

UI::UI(VkDevice* device) :
	device(device),
	renderPass(VK_NULL_HANDLE) {

	// ImGui context + style.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	// Need to check out to docking branch to use these 2 flags.
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();
}

UI::~UI() {}

void UI::createImguiRenderPass(const VkFormat format) {
	// -- 1.  Single colour attachment that we *load* (scene was blitted in)
	VkAttachmentDescription color{};
	color.format = format;
	color.samples = VK_SAMPLE_COUNT_1_BIT;          // UI needn’t be MSAA
	color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;     // keep the blit result
	color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;   // present later
	color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef{};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// -- 2.  Single‑subpass that draws ImGui
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;

	// -- 3.  Dependencies
	VkSubpassDependency depIn{};
	depIn.srcSubpass = VK_SUBPASS_EXTERNAL;
	depIn.dstSubpass = 0;
	depIn.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	depIn.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	depIn.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	depIn.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depOut{};
	depOut.srcSubpass = 0;
	depOut.dstSubpass = VK_SUBPASS_EXTERNAL;
	depOut.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	depOut.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	depOut.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	depOut.dstAccessMask = 0;

	std::array<VkSubpassDependency, 2> deps{ depIn, depOut };

	VkRenderPassCreateInfo rpInfo{};
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rpInfo.attachmentCount = 1;
	rpInfo.pAttachments = &color;
	rpInfo.subpassCount = 1;
	rpInfo.pSubpasses = &subpass;
	rpInfo.dependencyCount = static_cast<uint32_t>(deps.size());
	rpInfo.pDependencies = deps.data();

	if (vkCreateRenderPass(*device, &rpInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create UI render pass");
	}
}

void UI::createImguiFramebuffers(
	const std::vector<VulkanImage> swapChainImageViews,
	const uint32_t width,
	const uint32_t height) {
	const size_t size = swapChainImageViews.size();
	framebuffers.resize(size);

	for (size_t i = 0; i < size; ++i)
	{
		VkImageView attachments[] = { swapChainImageViews[i].view };

		VkFramebufferCreateInfo fbInfo{};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass = renderPass;
		fbInfo.attachmentCount = 1;
		fbInfo.pAttachments = attachments;
		fbInfo.width = width;
		fbInfo.height = height;
		fbInfo.layers = 1;

		if (vkCreateFramebuffer(*device, &fbInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create UI framebuffer");
	}
}

void UI::recordImguiRenderPass(
	VkCommandBuffer commandBuffer,
	uint32_t imageIndex,
	std::vector<VkImage> swapChainImages,
	VkImageLayout oldLayout,
	VkExtent2D extent) {
	cmdImageTransition(commandBuffer,
		swapChainImages[imageIndex],
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		oldLayout,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderPassBeginInfo rpBegin{};
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.renderPass = renderPass;
	rpBegin.framebuffer = framebuffers[imageIndex];
	rpBegin.renderArea.extent = extent;
	rpBegin.clearValueCount = 0; // we LOAD, so no clears

	vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

	// draw the UI
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRenderPass(commandBuffer);

	// COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
	cmdImageTransition(commandBuffer,
		swapChainImages[imageIndex],
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void UI::init(
	GLFWwindow* window,
	VkInstance instance,
	VkPhysicalDevice physicalDevice,
	uint32_t queryFamilyIndex,
	VkQueue queue,
	uint32_t minImageCount,
	uint32_t imageCount) {
	ImGui_ImplGlfw_InitForVulkan(window, true);
	// Create a small pool just for ImGui.
	VkDescriptorPoolSize sizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          100 },
		{ VK_DESCRIPTOR_TYPE_SAMPLER,                100 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         100 },
	};
	VkDescriptorPoolCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	info.maxSets = 100;
	info.poolSizeCount = static_cast<uint32_t>(std::size(sizes));
	info.pPoolSizes = sizes;
	if (vkCreateDescriptorPool(*device, &info, nullptr, &imguiPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create ImGui descriptor pool");

	// Vulkan backend init‑info.
	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = instance;
	initInfo.PhysicalDevice = physicalDevice;
	initInfo.Device = *device;
	initInfo.QueueFamily = queryFamilyIndex;
	initInfo.Queue = queue;
	initInfo.RenderPass = renderPass;
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.DescriptorPool = imguiPool;
	initInfo.Subpass = 0;
	initInfo.MinImageCount = minImageCount;
	initInfo.ImageCount = imageCount;
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.Allocator = nullptr;          // use default
	initInfo.CheckVkResultFn = [](VkResult err) {
		if (err != VK_SUCCESS)
			throw std::runtime_error("ImGui Vulkan backend error");
		};

	ImGui_ImplVulkan_Init(&initInfo);
}

void UI::destroy() {
	if (framebuffers.size() != 0) {
		for (VkFramebuffer fb : framebuffers) {
			vkDestroyFramebuffer(*device, fb, nullptr);
		}
		framebuffers.clear();
	}
	
	if (renderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(*device, renderPass, nullptr);
		renderPass = VK_NULL_HANDLE;
	}

	if (ImGui::GetCurrentContext() != nullptr)          
	{
		ImGuiIO& io = ImGui::GetIO();

		if (io.BackendRendererUserData) {
			ImGui_ImplVulkan_Shutdown();
		}
			
		if (io.BackendPlatformUserData) {
			ImGui_ImplGlfw_Shutdown();
		}	
	}
	
	if (imguiPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(*device, imguiPool, nullptr);
		imguiPool = VK_NULL_HANDLE;
	}
}

ImGuiIO& UI::getIO() {
	return ImGui::GetIO();
}

void UI::drawNewFrame() {
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// TODO: Draw UI elements.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::End();
	}

	ImGui::Render();
}

void UI::updateWindows() {
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}