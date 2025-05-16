#include "ui.h"

UI::UI() :
	device(nullptr) {}

UI::UI(VkDevice* device) :
	device(device) {
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

void UI::init(
	GLFWwindow* window,
	VkInstance instance,
	VkPhysicalDevice physicalDevice,
	uint32_t queryFamilyIndex,
	VkQueue queue,
	VkRenderPass renderPass,
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

void UI::drawNewFrame(UIState &state) {
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	{
		ImGui::Begin("Settings");

		ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

		ImGui::Text("Description: Currently the pipeline is using WBOIT for transparency rendering.");
		ImGui::Text("Transparency On ");
		ImGui::SameLine();
		ImGui::Checkbox("##Transparency", &state.transparencyOn);
		
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