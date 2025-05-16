#include "main.h"
#include "vertex.h"

#include <stb_image.h>

Main::Main(GLFWwindow* window,
	Camera* camera,
	std::unordered_map<std::string, std::vector<char>>&& shaders,
	std::unordered_map<std::string,
	std::pair<std::vector<Vertex>, std::vector<uint32_t>>>&& models,
	std::unordered_map<std::string, Image>&& textures,
	/*CubeMap&& envMap*/
	HDRImage&& envMap
)
	: window(window),
	camera(camera),
	shaders(shaders),
	models(models),
	textures(textures),
	envMap(envMap),
	physicalDevice(VK_NULL_HANDLE),
	msaaSamples(VK_SAMPLE_COUNT_1_BIT),
	currentFrame(0)
{
	initVulkan();
	// Must be called after vulkan is fully initalised.
	ui = UI(&device);
	uiState = {
		true // transparency on
	};
}

Main::~Main() {
	cleanUpVulkan();

	for (auto& pair : textures) {
		stbi_image_free(pair.second.pixels);
	}

	/*for (auto& face : envMap.faces) {
		delete[] face;
	}*/
	delete[] envMap.pixels;

	shaders.clear();
	models.clear();
	textures.clear();
	
	vertices.clear();
	indices.clear();

	uniformBuffers.clear();
	uniformBuffersMemory.clear();
	uniformBuffersMapped.clear();
	commandBuffers.clear();
	imageAvailableSemaphores.clear();
	renderFinishedSemaphores.clear();
	inFlightFences.clear();
	swapChainImages.clear();
	swapChainImageViews.clear();
	textureImages.clear();
}

void Main::initVulkan() {
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createCommandPool();

	// Create image views for the swapchain and offscreen
	createSwapChain();
	createSwapchainImageViews();
	createOffscreenImageResources();
	createTextureImages();
	createSampler(&textureSampler, textureMipLevels);
	/*createFlattenedEnvironmentMapImage();
	createSampler(&envMapSampler, 1.0f);*/
	createEnvMapImage(envMap);
	createSampler(&envMapSampler, envMapMipLevels, false, true);
	createUniformBuffers();

	createDescriptor();
	createRenderPasses();
	createPipelines();

	createFramebuffers();
	createVertexAndIndexBuffers();
	createCommandBuffers();
	createSyncObjects();
}

void Main::mainLoop() {
	createUI();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame(ui.getIO());
	}

	vkDeviceWaitIdle(device);
}

void Main::cleanUpVulkan() {
	vkDeviceWaitIdle(device);
	cleanupSwapChain();
	ImGui::DestroyContext();

	vkDestroySampler(device, textureSampler, nullptr);
	for (auto &pair : textureImages) {
		pair.second.destroy();
	}

	vkDestroySampler(device, envMapSampler, nullptr);
	envMapImage.destroy();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(device, uniformBuffers[i], nullptr);
		vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
	}

	for (auto& pair : vertices) {
		vkDestroyBuffer(device, pair.second.buffer, nullptr);
		vkFreeMemory(device, pair.second.memory, nullptr);
	}

	for (auto& pair : indices) {
		vkDestroyBuffer(device, pair.second.buffer, nullptr);
		vkFreeMemory(device, pair.second.memory, nullptr);
	}

	opaqueObjectsPipeline.destroy();
	opaqueObjectsRenderPass.destroy();
	opaqueHairPipeline.destroy();
	opaqueHairRenderPass.destroy();
	weightedColorPipeline.destroy();
	weightedRevealPipeline.destroy();
	transparentObjectsRenderPass.destroy();
	uiRenderPass.destroy();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyDevice(device, nullptr);
	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void Main::createInstance() {
	// Step 1: Create instance.
	// This data is technically optional, but it may provide some useful information to the driver in order to optimize our specific application.
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Main";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = 0;
	// Check for validation layer support.
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}
	// Add validation layers to the instance.
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	// Create the instance.
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

bool Main::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

    return true;
}

std::vector<const char*> Main::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

void Main::setupDebugMessenger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

VkResult Main::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Main::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void Main::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr; // Optional
}

void Main::createSurface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void Main::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	// Use an ordered map to automatically sort candidates by increasing score
	std::multimap<int, VkPhysicalDevice> candidates;

	for (const auto& device : devices) {
		int score = rateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	// Check if the best candidate is suitable at all
	if (candidates.rbegin()->first > 0) {
		physicalDevice = candidates.rbegin()->second;
		msaaSamples = getMaxUsableSampleCount();
	}
	else {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

int Main::rateDeviceSuitability(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Application can't function without geometry shaders
	if (!deviceFeatures.geometryShader || !deviceFeatures.samplerAnisotropy) {
		std::cout << "Geometry shader or samplerAnisotropy not supported on deivce: " << deviceProperties.deviceName << std::endl;
		return 0;
	}

	if (!deviceFeatures.independentBlend) {
		std::cout << "This device doesn't support different blend settings across different color attachments." << std::endl;
		return 0;
	}

	if (!findQueueFamilies(device).isComplete()) {
		std::cout << "Queue families not complete on device: " << deviceProperties.deviceName << std::endl;
		return 0;
	}

	bool extensionsSupported = checkDeviceExtensionSupport(device);
	if (!extensionsSupported) {
		std::cout << "Device extensions not supported on device: " << deviceProperties.deviceName << std::endl;
		return 0;
	}

	bool swapChainAdequate = false;
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
	swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	if (!swapChainAdequate) {
		std::cout << "Swap chain not adequate on device: " << deviceProperties.deviceName << std::endl;
		return 0;
	}

	return score;
}

QueueFamilyIndices Main::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		// Check if the queue family supports drawing commands.
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		// Check if the queue family supports presenting to the window surface.
		// Might not be the same queue family that supports drawing commands.
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

void Main::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { 
		indices.graphicsFamily.value(), 
		indices.presentFamily.value() 
	};
	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.geometryShader = VK_TRUE;
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	// MSAA only smoothens out the edges of geometry but not the interior filling. 
	// This may lead to a situation when you get a smooth polygon rendered on screen 
	// but the applied texture will still look aliased if it contains high contrasting colors. 
	// One way to approach this problem is to enable Sample Shading 
	// which will improve the image quality even further, though at an additional performance cost.
	deviceFeatures.sampleRateShading = VK_TRUE;
	deviceFeatures.independentBlend = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	// enabledLayerCount and ppEnabledLayerNames fields of VkDeviceCreateInfo are ignored by up-to-date implementations. 
	// However, it is still a good idea to set them anyway to be compatible with older implementations.
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}


	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

bool Main::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails Main::querySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR Main::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR Main::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Main::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		// We don't use our class variables WIDTH and HEIGHT here because those are screen coordinates and we need pixels.
		// These two won't match, for example, in Apple's Retina displays where the resolution of the window in pixel will be larger than the resolution in screen coordinates.
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void Main::createSwapChain() {
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
	// Simply sticking to this minimum means that we may sometimes have to wait on the driver to 
	// complete internal operations before we can acquire another image to render to. 
	// Therefore it is recommended to request at least one more image than the minimum.
	/*uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;*/
	imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Main::createSwapchainImageViews() {
	swapChainImageViews.resize(swapChainImages.size());

	for (uint32_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = VulkanImage(
			&device,
			swapChainImages[i],
			swapChainImageFormat,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1
		);

		swapChainImageViews[i].createView();
	}
}

void Main::createDescriptor() {
	descriptor = Descriptor(
		&device,
		1, // numUniformBuffers
		textureImages.size(), // numTextureBuffers: numTextures + 1 for envMap
		2 // numInputBuffers
	);

	// Add descriptor bindings.
	descriptor.addDescriptorSetLayoutBinding(
		BIND_UBO,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT,
		uniformBuffers
	);
	descriptor.addDescriptorSetLayoutBinding(
		BIND_WBOIT_COLOR,
		VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		{},
		weightedColorImage.view
	);
	descriptor.addDescriptorSetLayoutBinding(
		BIND_WBOIT_REVEAL,
		VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		{},
		weightedRevealImage.view
	);

	for (auto& pair : textureImages) {
		auto tmp = pair.first.substr(pair.first.find('_') + 1);
		descriptor.addDescriptorSetLayoutBinding(
			static_cast<uint32_t>(std::stoul(tmp)),
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			{},
			pair.second.view,
			textureSampler
		);
	}

	//descriptor.addDescriptorSetLayoutBinding(
	//	BIND_ENV_MAP,
	//	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	//	VK_SHADER_STAGE_FRAGMENT_BIT,
	//	{},
	//	envMapImage.view,
	//	envMapSampler
	//);

	descriptor.create();
}

void Main::createPipelines() {
	/* opaqueObjectsPipeline */
	opaqueObjectsPipeline = Pipeline(
		&device,
		opaqueObjectsRenderPass
	);
	opaqueObjectsPipeline.createPipelineLayout(&descriptor.descriptorSetLayout);
	
	VkPipelineRasterizationStateCreateInfo opaqueRasterizer{};
	opaqueRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	opaqueRasterizer.depthClampEnable = VK_FALSE;
	opaqueRasterizer.rasterizerDiscardEnable = VK_FALSE;
	opaqueRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	opaqueRasterizer.lineWidth = 1.0f;
	opaqueRasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	opaqueRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	opaqueRasterizer.depthBiasEnable = VK_FALSE;
	opaqueRasterizer.depthBiasConstantFactor = 0.0f;
	opaqueRasterizer.depthBiasClamp = 0.0f;
	opaqueRasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineDepthStencilStateCreateInfo opaqueDepthStencil{};
	opaqueDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	opaqueDepthStencil.depthTestEnable = VK_TRUE;
	opaqueDepthStencil.depthWriteEnable = VK_TRUE;
	opaqueDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	opaqueDepthStencil.depthBoundsTestEnable = VK_FALSE;
	opaqueDepthStencil.minDepthBounds = 0.0f;
	opaqueDepthStencil.maxDepthBounds = 1.0f;
	opaqueDepthStencil.stencilTestEnable = VK_FALSE;
	opaqueDepthStencil.front = {};
	opaqueDepthStencil.back = {};

	const VkColorComponentFlags colorFlags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendAttachmentState opaqueColorBlendAttachment{};
	opaqueColorBlendAttachment.colorWriteMask = colorFlags;
	opaqueColorBlendAttachment.blendEnable = VK_FALSE;
	opaqueColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	opaqueColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	opaqueColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	opaqueColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	opaqueColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	opaqueColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	opaqueObjectsPipeline.createPipeline(
		shaders["vertShader"],
		shaders["opaqueFragShader"],
		false,
		opaqueRasterizer,
		msaaSamples,
		opaqueDepthStencil,
		{ opaqueColorBlendAttachment },
		0
	);
	
	
	/* weightedColorPipeline */
	weightedColorPipeline = Pipeline(
		&device,
		transparentObjectsRenderPass
	);
	weightedColorPipeline.createPipelineLayout(&descriptor.descriptorSetLayout);

	VkPipelineRasterizationStateCreateInfo weightedColorRasterizer = opaqueRasterizer;
	weightedColorRasterizer.cullMode = VK_CULL_MODE_NONE;

	VkPipelineDepthStencilStateCreateInfo weightedColorDepthStencil = opaqueDepthStencil;
	weightedColorDepthStencil.depthWriteEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState weightedColorBlendAttachment0{};
	weightedColorBlendAttachment0.colorWriteMask = colorFlags;
	weightedColorBlendAttachment0.blendEnable = VK_TRUE;
	weightedColorBlendAttachment0.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	weightedColorBlendAttachment0.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	weightedColorBlendAttachment0.colorBlendOp = VK_BLEND_OP_ADD;
	weightedColorBlendAttachment0.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	weightedColorBlendAttachment0.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	weightedColorBlendAttachment0.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendAttachmentState weightedColorBlendAttachment1{};
	weightedColorBlendAttachment1.colorWriteMask = colorFlags;
	weightedColorBlendAttachment1.blendEnable = VK_TRUE;
	weightedColorBlendAttachment1.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	weightedColorBlendAttachment1.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	weightedColorBlendAttachment1.colorBlendOp = VK_BLEND_OP_ADD;
	weightedColorBlendAttachment1.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	weightedColorBlendAttachment1.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	weightedColorBlendAttachment1.alphaBlendOp = VK_BLEND_OP_ADD;
	weightedColorPipeline.createPipeline(
		shaders["vertShader"],
		shaders["weightedColorFragShader"],
		false,
		weightedColorRasterizer,
		msaaSamples,
		weightedColorDepthStencil,
		{ weightedColorBlendAttachment0, weightedColorBlendAttachment1 },
		0
	);

	/* weightedRevealPipeline */
	weightedRevealPipeline = Pipeline(
		&device,
		transparentObjectsRenderPass
	);
	weightedRevealPipeline.createPipelineLayout(&descriptor.descriptorSetLayout);
	VkPipelineColorBlendAttachmentState weightedRevealColorBlendAttachment{};
	weightedRevealColorBlendAttachment.colorWriteMask = colorFlags;
	weightedRevealColorBlendAttachment.blendEnable = VK_TRUE;
	weightedRevealColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	weightedRevealColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	weightedRevealColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	weightedRevealColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	weightedRevealColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	weightedRevealColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	weightedRevealPipeline.createPipeline(
		shaders["triangleShader"], 
		shaders["weightedRevealFragShader"],
		true,
		weightedColorRasterizer,
		msaaSamples,
		weightedColorDepthStencil,
		{ weightedRevealColorBlendAttachment },
		1
	);

	/* opaqueHairPipeline */
	opaqueHairPipeline = Pipeline(
		&device,
		opaqueHairRenderPass
	);
	opaqueHairPipeline.createPipelineLayout(&descriptor.descriptorSetLayout);
	opaqueHairPipeline.createPipeline(
		shaders["vertShader"],
		shaders["opaqueHairFragShader"],
		false,
		weightedColorRasterizer,
		msaaSamples,
		opaqueDepthStencil,
		{ opaqueColorBlendAttachment },
		0
	);
}

void Main::createOpaqueObjectsFramebuffer() {
	std::array<VkImageView, 2> attachments = { offscreenColorImage.view, depthImage.view };
	VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.renderPass = opaqueObjectsRenderPass.renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = offscreenColorImage.width;
	framebufferInfo.height = offscreenColorImage.height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &opaqueObjectsFramebuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create opaque objects framebuffer!");
	}
}

void Main::createTransparentObjectsFramebuffer() {
	std::array<VkImageView, 4> attachments = { weightedColorImage.view, weightedRevealImage.view, offscreenColorImage.view, depthImage.view };

	VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.renderPass = transparentObjectsRenderPass.renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = weightedColorImage.width;
	framebufferInfo.height = weightedColorImage.height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &transparentObjectsFramebuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create transparent objects framebuffer!");
	}
}

void Main::createSwapchainFramebuffers() {
	// Create swapchain frame buffers.
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		std::array<VkImageView, 4> attachments = {
			// The order here needs to match the attachment order in transparentObjectsRenderPass
			weightedColorImage.view,
			weightedRevealImage.view,
			offscreenColorImage.view,
			depthImage.view
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		// TODO: Note that you should use the render pass whose subpass writes directly to the swapchain image view.
		// So this might be transparentObjectRenderPass
		framebufferInfo.renderPass = transparentObjectsRenderPass.renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swapchain framebuffers!");
		}
	}	
}

void Main::recordDrawForMesh(VkCommandBuffer cmd, const std::string& name, Pipeline &pipeline) {
	const VkBuffer vbuf = vertices.at(name).buffer;
	const VkBuffer ibuf = indices.at(name).buffer;
	const uint32_t indexCnt = static_cast<uint32_t>(indices.at(name).count);

	const VkDeviceSize offset = 0;

	//vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
	//	opaqueObjectsPipeline.pipeline);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

	vkCmdBindVertexBuffers(cmd, 0, 1, &vbuf, &offset);
	vkCmdBindIndexBuffer(cmd, ibuf, 0, VK_INDEX_TYPE_UINT32);

	/*vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
		opaqueObjectsPipeline.layout,
		0, 1,
		&descriptor.descriptorSets[currentFrame],
		0, nullptr);*/
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline.layout,
		0, 1,
		&descriptor.descriptorSets[currentFrame],
		0, nullptr);

	vkCmdDrawIndexed(cmd, indexCnt, 1, 0, 0, 0);
}

void Main::createFramebuffers() {
	createSwapchainFramebuffers();
	createUIFramebuffers();
	createOpaqueObjectsFramebuffer();
	createTransparentObjectsFramebuffer();
}

void Main::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void Main::createCommandBuffers() {
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

VkCommandBuffer Main::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Main::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Main::recordOpaqueObjectsRenderPass(VkCommandBuffer commandBuffer) {
	offscreenColorImage.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	
	// Set up the render pass
	VkRenderPassBeginInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassInfo.renderPass = opaqueObjectsRenderPass.renderPass;
	renderPassInfo.framebuffer = opaqueObjectsFramebuffer;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = offscreenColorImage.width;
	renderPassInfo.renderArea.extent.height = offscreenColorImage.height;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.5f, 0.5f, 0.5f, 1.0f };  
	clearValues[1].depthStencil = { 1.0f, 0 };               
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// We need to do this because we've set the viewport and scissor to be dynamic.
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	if (uiState.transparencyOn) {
		recordDrawForMesh(commandBuffer, "head", opaqueObjectsPipeline);
	}
	else {
		recordDrawForMesh(commandBuffer, "head", opaqueObjectsPipeline);
		recordDrawForMesh(commandBuffer, "hair", opaqueHairPipeline);
	}
	
	vkCmdEndRenderPass(commandBuffer);
}

void Main::recordTransparentObjectsRenderPass(VkCommandBuffer commandBuffer) {
	offscreenColorImage.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	VkRenderPassBeginInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassInfo.renderPass = transparentObjectsRenderPass.renderPass;
	renderPassInfo.framebuffer = transparentObjectsFramebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = weightedColorImage.width;
	renderPassInfo.renderArea.extent.height = weightedRevealImage.height;
	std::array<VkClearValue, 2> clearValues;
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f }; 
	clearValues[1].color.float32[0] = 1.0f;  // Initially, all pixels show through all the way (reveal = 100%)
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// COLOR PASS
	// Bind the vertex and index buffers
	VkBuffer vertexBuffers[] = { vertices["hair"].buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indices["hair"].buffer, 0, VK_INDEX_TYPE_UINT32);

	// We need to do this because we've set the viewport and scissor to be dynamic.
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// Computes the weighted sum and reveal factor.
	/*vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, weightedColorPipeline);*/
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, weightedColorPipeline.pipeline);
	// Draw all objects
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices["hair"].count), 1, 0, 0, 0);
	
	// Move to the next subpass
	vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
	// COMPOSITE PASS
	// Averages out the summed colors (in some sense) to get the final transparent color.
	/*vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, weightedRevealPipeline);*/
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, weightedRevealPipeline.pipeline);
	// Draw a full-screen triangle
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	
	vkCmdEndRenderPass(commandBuffer);
}

void Main::recordSwapchainBlit(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	assert(offscreenColorImage.currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	assert(offscreenColorImage.currentAccesses == (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));
	offscreenColorImage.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT);

	// So far we only handle multi-sampling
	if (msaaSamples != 1) {
		downsampleImage.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT);
		
		// Resolve the MSAA image m_colorImage to m_downsampleImage
		VkImageResolve resolveRegion = { 0 };  
		resolveRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		resolveRegion.srcSubresource.layerCount = 1;
		resolveRegion.dstSubresource = resolveRegion.srcSubresource;
		resolveRegion.extent = { offscreenColorImage.width, offscreenColorImage.height, 1 };

		vkCmdResolveImage(commandBuffer, offscreenColorImage.image, offscreenColorImage.currentLayout, downsampleImage.image, downsampleImage.currentLayout, 1, &resolveRegion);                      

		downsampleImage.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT);
	}
	else {
		throw std::runtime_error("Not handled yet.");
	}

	// Blit the downsampled image to the swapchain
	VkImageBlit blitRegion = { 0 };
	blitRegion.srcOffsets[1].x = downsampleImage.width;
	blitRegion.srcOffsets[1].y = downsampleImage.height;
	blitRegion.srcOffsets[1].z = 1;
	blitRegion.dstOffsets[1].x = swapChainExtent.width;
	blitRegion.dstOffsets[1].y = swapChainExtent.height;
	blitRegion.dstOffsets[1].z = 1;
	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.layerCount = 1;

	cmdImageTransition(commandBuffer, swapChainImages[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkCmdBlitImage(commandBuffer, downsampleImage.image, downsampleImage.currentLayout, swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);                 
	cmdImageTransition(commandBuffer, swapChainImages[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	
	offscreenColorImage.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
}

void Main::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; 
	beginInfo.pInheritanceInfo = nullptr; 

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	// TODO: Draw envmap and everything related
	
	// Draw opaque objects.
	recordOpaqueObjectsRenderPass(commandBuffer);
	// Draw transparent objects.
	if (uiState.transparencyOn) {
		recordTransparentObjectsRenderPass(commandBuffer);
	}
	// Blit to swapchain.
	recordSwapchainBlit(commandBuffer, imageIndex);
	// Draw UI.
	recordUIRenderPass(commandBuffer, imageIndex);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer!");
	}
}

void Main::createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void Main::drawFrame(ImGuiIO& io) {
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	vkResetFences(device, 1, &inFlightFences[currentFrame]);
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);

	ui.drawNewFrame(uiState);

	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	updateUniformBuffer(currentFrame);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	ui.updateWindows();

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || camera->resized) {
		camera->resized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Main::cleanupSwapChain() {
	offscreenColorImage.destroy();
	depthImage.destroy();
	weightedColorImage.destroy();
	weightedRevealImage.destroy();
	downsampleImage.destroy();
	ui.destroy();

	// Destroy framebuffers.
	for (VkFramebuffer fb : uiFramebuffers) {
		vkDestroyFramebuffer(device, fb, nullptr);
	}
	uiFramebuffers.clear();

	for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
	}
	vkDestroyFramebuffer(device, opaqueObjectsFramebuffer, nullptr);
	vkDestroyFramebuffer(device, transparentObjectsFramebuffer, nullptr);

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		swapChainImageViews[i].destroySwapchainView();
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);

	descriptor.destroy();
}

void Main::recreateSwapChain() {
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	cleanupSwapChain();

	createSwapChain();
	createSwapchainImageViews();
	createOffscreenImageResources();
	createFramebuffers();
	createDescriptor();
	createUI();
}

uint32_t Main::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type!");
}

void Main::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

void Main::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

MeshBuffer Main::createVertexBuffer(std::vector<Vertex> vertices) {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	return MeshBuffer(vertexBuffer, vertexBufferMemory, vertices.size());
}

MeshBuffer Main::createIndexBuffer(std::vector<uint32_t> indices) {
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	return MeshBuffer(indexBuffer, indexBufferMemory, indices.size());
}

void Main::createVertexAndIndexBuffers() {
	for (auto& pair : models) {
		const auto curVertices = pair.second.first;
		const auto curIndices = pair.second.second;
		vertices[pair.first] = createVertexBuffer(curVertices);
		indices[pair.first] = createIndexBuffer(curIndices);
	}
}

void Main::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

		vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
	}
}

void Main::updateUniformBuffer(uint32_t currentImage) {
	UniformBufferObject ubo{};
	ubo.model = glm::mat4(1.0f);
	ubo.view = camera->view;
	ubo.proj = camera->proj;
	// GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. 
	// The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. 
	// If you don't do this, then the image will be rendered upside down.
	ubo.proj[1][1] *= -1;
	ubo.cameraPos = camera->position;

	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Main::transitionImage(
	VulkanImage image, 
	VkImageLayout newLayout, 
	VkAccessFlags newtAccesses, 
	VkImageSubresourceRange range
) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	image.transitionLayout(commandBuffer, newLayout, newtAccesses, range);
	endSingleTimeCommands(commandBuffer);
}

VulkanImage Main::createTextureImageGeneric(
	int texWidth,
	int texHeight,
	const void* pixels,
	uint32_t* mipLevels,
	VkFormat format,
	size_t bytesPerChannel) {
	constexpr uint32_t CHANNELS = 4;               // we always upload RGBA
	VkDeviceSize imageSize = texWidth * texHeight * CHANNELS * bytesPerChannel;
	*mipLevels =
		static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	/* ---------- staging buffer ---------- */
	VkBuffer       stagingBuffer;
	VkDeviceMemory stagingMem;
	createBuffer(imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingMem);

	void* data = nullptr;
	vkMapMemory(device, stagingMem, 0, imageSize, 0, &data);
	std::memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingMem);

	/* ---------- image object (device local) ---------- */
	VulkanImage texture(
		&device,
		texWidth,
		texHeight,
		*mipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		format,                                         
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT);

	texture.createImage();

	VkMemoryRequirements memReq{};
	vkGetImageMemoryRequirements(device, texture.image, &memReq);
	texture.bindMemory(memReq.size,
		findMemoryType(memReq.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	transitionImage(texture,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT);

	copyBufferToImage(stagingBuffer, texture.image,
		static_cast<uint32_t>(texWidth),
		static_cast<uint32_t>(texHeight));

	generateMipmaps(texture.image, format, texWidth, texHeight, *mipLevels);

	texture.createView();

	/* ---------- cleanup ---------- */
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingMem, nullptr);

	return texture;
}

VulkanImage Main::createTextureImage(int texWidth, int texHeight, stbi_uc* pixels) {
	//// Create staging buffer and copy the image data to it
	//VkDeviceSize imageSize = texWidth * texHeight * 4;
	//textureMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	//VkBuffer stagingBuffer;
	//VkDeviceMemory stagingBufferMemory;
	//createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	//
	//void* data;
	//vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	//memcpy(data, pixels, static_cast<size_t>(imageSize));
	//vkUnmapMemory(device, stagingBufferMemory);

	//// Create image and transition it to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	//VulkanImage textureImage = VulkanImage(
	//	&device, 
	//	texWidth, 
	//	texHeight,
	//	textureMipLevels,
	//	VK_SAMPLE_COUNT_1_BIT,
	//	VK_FORMAT_R8G8B8A8_SRGB,
	//	VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	//	VK_IMAGE_ASPECT_COLOR_BIT
	//);
	//textureImage.createImage();

	//VkMemoryRequirements memRequirements;
	//vkGetImageMemoryRequirements(device, textureImage.image, &memRequirements);
	//textureImage.bindMemory(memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	//transitionImage(textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT);

	//// Copy the buffer to the image
	//copyBufferToImage(stagingBuffer, textureImage.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	//generateMipmaps(textureImage.image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, textureMipLevels);
	//
	//textureImage.createView();

	//// Cleanup
	//vkDestroyBuffer(device, stagingBuffer, nullptr);
	//vkFreeMemory(device, stagingBufferMemory, nullptr);

	//return textureImage;
	return createTextureImageGeneric(texWidth,
		texHeight,
		pixels,
		&textureMipLevels,
		VK_FORMAT_R8G8B8A8_SRGB,
		/* bytesPerChannel = */ 1);
}

void Main::createTextureImages() {
	for (auto &pair : textures) {
		const auto image = pair.second;
		textureImages[pair.first] = createTextureImage(image.width, image.height, image.pixels);
	}
}

void Main::createFlattenedEnvironmentMapImage(CubeMap flattenedEnvMap, bool useHigherPrecision) {
	const VkFormat format = useHigherPrecision ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R16G16B16A16_SFLOAT;
	const size_t bytesPerComponent = useHigherPrecision ? sizeof(float) : sizeof(uint16_t);
	const size_t bytesPerPixel = 4 * bytesPerComponent;   // RGBA

	// This is basically an image (view) with 6 layers because Vulkan doesn't have cubemap.
	envMapImage = VulkanImage(
		&device,
		flattenedEnvMap.resolution,
		flattenedEnvMap.resolution,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		format,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		6,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE
	);

	envMapImage.createImage();

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, envMapImage.image, &memRequirements);
	envMapImage.bindMemory(memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	envMapImage.createView();

	VkImageSubresourceRange cubeRange{};
	cubeRange.baseMipLevel = 0;
	cubeRange.levelCount = 1;
	cubeRange.baseArrayLayer = 0;
	cubeRange.layerCount = 6;

	transitionImage(envMapImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // new layout
		VK_ACCESS_TRANSFER_WRITE_BIT,          // dstAccess
		cubeRange);

	// Create staging buffer
	VkDeviceSize sizePerLayer = flattenedEnvMap.resolution * flattenedEnvMap.resolution * bytesPerPixel; // 4 bytes/channel per pixel
	VkDeviceSize totalSize = sizePerLayer * 6;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(
		totalSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	uint8_t* dst = nullptr;
	vkMapMemory(device, stagingBufferMemory, 0, totalSize, 0, reinterpret_cast<void**>(&dst));

	for (uint32_t face = 0; face < 6; ++face) {
		// Copy each face to its allocated memory
		const float* src = flattenedEnvMap.faces[face];               // RGB[A] 32F
		

		const size_t pixelCount = static_cast<size_t>(flattenedEnvMap.resolution) * flattenedEnvMap.resolution;
		for (size_t i = 0; i < pixelCount; ++i)
		{
			const float r = src[i * envMap.channels + 0];
			const float g = src[i * envMap.channels + 1];
			const float b = src[i * envMap.channels + 2];
			const float a = (envMap.channels == 4) ?
				src[i * envMap.channels + 3] : 1.0f;

			if (useHigherPrecision) {
				float* dstFace = reinterpret_cast<float*>(dst + face * sizePerLayer);
				dstFace[i * 4 + 0] = r;   // float
				dstFace[i * 4 + 1] = g;   // float
				dstFace[i * 4 + 2] = b;   // float
				dstFace[i * 4 + 3] = a;   // float (1.0f if the HDR is RGB only)
			}
			else {
				uint16_t* dstFace = reinterpret_cast<uint16_t*>(dst + face * sizePerLayer);
				// If you later need the original value (within half‑precision error), call glm::unpackHalf1x16().
				dstFace[i * 4 + 0] = glm::packHalf1x16(r);
				dstFace[i * 4 + 1] = glm::packHalf1x16(g);
				dstFace[i * 4 + 2] = glm::packHalf1x16(b);
				dstFace[i * 4 + 3] = glm::packHalf1x16(a);
			}
		}
	}

	// Unmap the memory
	vkUnmapMemory(device, stagingBufferMemory);

	// Copy the buffer to the image and transition the image back to the shader read layout
	copyBufferToImage(stagingBuffer, envMapImage.image, flattenedEnvMap.resolution, flattenedEnvMap.resolution, 6);
	transitionImage(envMapImage,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_SHADER_READ_BIT,
		cubeRange);

	// Destrpy the staging buffer and clear the memory
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Main::createEnvMapImage(const HDRImage& envMap) {
	// Ensure the pixel block is RGBA ‑ if stb_image gave you RGB,
	// expand it beforehand or switch to VK_FORMAT_R32G32B32_SFLOAT.
	envMapImage = createTextureImageGeneric(envMap.width,
		envMap.height,
		envMap.pixels,
		&envMapMipLevels,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		/* bytesPerChannel = */ sizeof(float));
}

void Main::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layers) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layers;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	endSingleTimeCommands(commandBuffer);
}

void Main::createSampler(VkSampler *sampler, float mipLevels, bool useNearestFilter, bool isEnvMap) {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	if (useNearestFilter) {
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
	}
	else {
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
	}
	if (isEnvMap) {
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
	else {
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
	samplerInfo.anisotropyEnable = VK_TRUE;
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = mipLevels;
	//samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

	if (vkCreateSampler(device, &samplerInfo, nullptr, sampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

VkFormat Main::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}

		throw std::runtime_error("failed to find supported format!");
	}
}

VkFormat Main::findDepthFormat() {
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool Main::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// TODO: It should be noted that it is uncommon in practice to generate the mipmap levels at runtime anyway. 
// Usually they are pregenerated and stored in the texture file alongside the base level to improve loading speed.
void Main::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;
	
	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) {
			mipWidth /= 2;
		}

		if (mipHeight > 1) {
			mipHeight /= 2;
		}	
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

VkSampleCountFlagBits Main::getMaxUsableSampleCount() {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

void Main::createImageResource(VulkanImage *image, VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags) {
	*image = VulkanImage(
		&device,
		swapChainExtent.width,
		swapChainExtent.height,
		1,
		samples,
		format,
		usage,
		aspectFlags
	);
	image->createImage();

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image->image, &memRequirements);
	image->bindMemory(memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	image->createView();
}

void Main::createOffscreenImageResources() {
	//VK_FORMAT_B8G8R8A8_SRGB
	createImageResource(&offscreenColorImage, VK_FORMAT_R8G8B8A8_SRGB, msaaSamples, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	transitionImage(offscreenColorImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	createImageResource(&depthImage, findDepthFormat(), msaaSamples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	transitionImage(depthImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

	createImageResource(&weightedColorImage, VK_FORMAT_R16G16B16A16_SFLOAT, msaaSamples, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	transitionImage(weightedColorImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	createImageResource(&weightedRevealImage, VK_FORMAT_R16_SFLOAT, msaaSamples, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	transitionImage(weightedRevealImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	createImageResource(&downsampleImage, offscreenColorImage.format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Main::createRenderPasses() {
	/* Opaque objects render pass */
	opaqueObjectsRenderPass = RenderPass(&device);
	// Add attachments
	{
		// Color attachment
		opaqueObjectsRenderPass.addAttachment(
			offscreenColorImage.format,
			msaaSamples,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
		// Depth attachment
		opaqueObjectsRenderPass.addAttachment(
			depthImage.format,
			msaaSamples,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		);
	}
	
	// Add subpass
	{
		opaqueObjectsRenderPass.addSubpass(
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			{
				{0, AttachmentType::COLOR},
				{1, AttachmentType::DEPTH}
			}
		);
	}
	
	// Add dependency
	{
		VkSubpassDependency selfDependency;
		selfDependency.srcSubpass = 0;
		selfDependency.dstSubpass = 0;
		selfDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		selfDependency.dstStageMask = selfDependency.srcStageMask;
		selfDependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		selfDependency.dstAccessMask = selfDependency.srcAccessMask;
		selfDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;  // Required, since we use framebuffer-space stages
		opaqueObjectsRenderPass.createRenderPass({ selfDependency });
	}
	
	/* Transparent objects render pass */
	transparentObjectsRenderPass = RenderPass(&device);
	// Add attachments
	{
		// weightedColorAttachment
		transparentObjectsRenderPass.addAttachment(
			weightedColorImage.format,
			msaaSamples,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
		// weightedRevealAttachment
		transparentObjectsRenderPass.addAttachment(
			weightedRevealImage.format,
			msaaSamples,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
		// colorAttachment
		transparentObjectsRenderPass.addAttachment(
			offscreenColorImage.format,
			msaaSamples,
			VK_ATTACHMENT_LOAD_OP_LOAD,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
		// depthAttachment
		transparentObjectsRenderPass.addAttachment(
			depthImage.format,
			msaaSamples,
			VK_ATTACHMENT_LOAD_OP_LOAD,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		);
	}
	
	// Add subpasses
	{
		// Subpass 0
		transparentObjectsRenderPass.addSubpass(
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			{
				{0, AttachmentType::COLOR},
				{1, AttachmentType::COLOR},
				{3, AttachmentType::DEPTH}
			}
		);

		// Subpass 1
		transparentObjectsRenderPass.addSubpass(
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			{
				{0, AttachmentType::INPUT},
				{1, AttachmentType::INPUT},
				{2, AttachmentType::COLOR}
			}
		);
	}

	// Add dependencies
	{
		std::vector<VkSubpassDependency> subpassDependencies(3);
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].srcAccessMask = 0;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].dstSubpass = 1;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		// Finally, we have a dependency at the end to allow the images to transition back to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		subpassDependencies[2].srcSubpass = 1;
		subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[2].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		transparentObjectsRenderPass.createRenderPass(subpassDependencies);
	}

	/* UI render pass */
	uiRenderPass = RenderPass(&device);
	// Add attachment
	{
		uiRenderPass.addAttachment(
			swapChainImageFormat,
			VK_SAMPLE_COUNT_1_BIT, // UI needn’t be MSAA
			VK_ATTACHMENT_LOAD_OP_LOAD, // keep the blit result
			VK_ATTACHMENT_STORE_OP_STORE, // present later
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
	}

	// Add subpass
	{
		uiRenderPass.addSubpass(
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			{
				{0, AttachmentType::COLOR}
			}
		);
	}

	// Add dependencies
	{
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
		uiRenderPass.createRenderPass({ depIn, depOut });
	}

	/* Opaque hair render pass */
	opaqueHairRenderPass = RenderPass(&device);
	// Add attachaments
	{
		// Color attachment
		opaqueHairRenderPass.addAttachment(
			offscreenColorImage.format,
			msaaSamples,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
		// Depth attachment
		opaqueHairRenderPass.addAttachment(
			depthImage.format,
			msaaSamples,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		);
	}
	// Add subpass
	{
		opaqueHairRenderPass.addSubpass(
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			{
				{0, AttachmentType::COLOR},
				{1, AttachmentType::DEPTH}
			}
		);
	}
	// Add dependency
	{
		VkSubpassDependency selfDependency;
		selfDependency.srcSubpass = 0;
		selfDependency.dstSubpass = 0;
		selfDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		selfDependency.dstStageMask = selfDependency.srcStageMask;
		selfDependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		selfDependency.dstAccessMask = selfDependency.srcAccessMask;
		selfDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;  // Required, since we use framebuffer-space stages
		opaqueHairRenderPass.createRenderPass({ selfDependency });
	}
	
}

void Main::createUIFramebuffers() {
	const size_t size = swapChainImageViews.size();
	uiFramebuffers.resize(size);

	for (size_t i = 0; i < size; ++i)
	{
		VkImageView attachments[] = { swapChainImageViews[i].view };

		VkFramebufferCreateInfo fbInfo{};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass = uiRenderPass.renderPass;
		fbInfo.attachmentCount = 1;
		fbInfo.pAttachments = attachments;
		fbInfo.width = swapChainExtent.width;
		fbInfo.height = swapChainExtent.height;
		fbInfo.layers = 1;

		if (vkCreateFramebuffer(device, &fbInfo, nullptr, &uiFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create UI framebuffer");
	}
}

void Main::createUI() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	ui.init(
		window,
		instance,
		physicalDevice,
		indices.graphicsFamily.value(),
		graphicsQueue,
		uiRenderPass.renderPass,
		imageCount,
		swapChainImages.size()
	);
}

void Main::recordUIRenderPass(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	cmdImageTransition(
		commandBuffer,
		swapChainImages[imageIndex],
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // TODO: for now this is harded but should be recorded from previous transitions
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);

	VkRenderPassBeginInfo rpBegin{};
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.renderPass = uiRenderPass.renderPass;
	rpBegin.framebuffer = uiFramebuffers[imageIndex];
	rpBegin.renderArea.extent = swapChainExtent;
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