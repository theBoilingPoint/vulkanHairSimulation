#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <optional>
#include <cstdint> 
#include <limits> 
#include <algorithm> 
#include <unordered_map>

#include "bindings.inc"
#include "ui.h"
#include "camera.h"
#include "vertex.h"
#include "vulkanImage.h"
#include "descriptor.h"
#include "renderPass.h"
#include "pipeline.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	// Basic surface capabilities(min / max number of images in swap chain, 
	// min / max width and height of images).
	VkSurfaceCapabilitiesKHR capabilities;
	// Surface formats (pixel format, color space).
	std::vector<VkSurfaceFormatKHR> formats;
	// Available presentation modes.
	std::vector<VkPresentModeKHR> presentModes;
};

struct MeshBuffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	uint32_t count;
};

class Main {
public:
	Main(GLFWwindow* window,
		Camera* camera,
		std::unordered_map<std::string, std::vector<char>>&& shaders,
		std::unordered_map<std::string,
		std::pair<std::vector<Vertex>, std::vector<uint32_t>>>&& models,
		std::unordered_map<std::string, Image>&& textures = {},
		//CubeMap&& envMap = {}
		HDRImage&& envMap = {});

	~Main();

	void mainLoop();

private:
	// UI
	UI ui;
	UIState uiState;

	// Vulkan
	GLFWwindow* window;
	Camera* camera;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;
	// This object will be implicitly destroyed when the VkInstance is destroyed, 
	// so we won't need to do anything new in the cleanup function.
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	// Device queues are implicitly cleaned up when the device is destroyed, so we don't need to do anything in cleanup.
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	uint32_t imageCount;

	ImGui_ImplVulkanH_Window g_MainWindowData;

	VkCommandPool commandPool;

	std::unordered_map<std::string, std::vector<char>> shaders;
	std::unordered_map<std::string, std::pair<std::vector<Vertex>, std::vector<uint32_t>>> models;
	std::unordered_map<std::string, Image> textures;
	// CubeMap envMap;
	HDRImage envMap;

	std::unordered_map<std::string, MeshBuffer> vertices;
	std::unordered_map<std::string, MeshBuffer> indices;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	Descriptor descriptor;

	// Command buffers will be automatically freed when their command pool is destroyed, so we don't need explicit cleanup.
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	// The images were created by the implementation for the swap chain and they will be automatically cleaned up once the swap chain has been destroyed
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	VkSampler envMapSampler;
	uint32_t envMapMipLevels;
	VkSampler textureSampler;
	uint32_t textureMipLevels;
	VkSampleCountFlagBits msaaSamples;

	std::vector<VulkanImage> swapChainImageViews;
	std::unordered_map<std::string, VulkanImage> textureImages;
	VulkanImage envMapImage;
	VulkanImage depthImage;
	VulkanImage offscreenColorImage;
	VulkanImage weightedColorImage;
	VulkanImage weightedRevealImage;
	// Given that we use multisampling for images before this step, we need to downsample it before blitting the result to the swapchain
	VulkanImage downsampleImage;

	// TODO: EnvMap
	// STORAGE cubemaps: VulkanImage envMapStorageImage;
	// SAMPLER cubemaps: VulkanImage envMapSamplerImage;

	// Opaque Objects
	RenderPass opaqueObjectsRenderPass;
	VkFramebuffer opaqueObjectsFramebuffer;
	VkPipelineLayout opaqueObjectsPipelineLayout;
	VkPipeline opaqueObjectsPipeline;	
	//Pipeline opaqueObjectsPipeline;

	// Transparent Objects
	RenderPass transparentObjectsRenderPass;

	VkFramebuffer transparentObjectsFramebuffer;
	VkPipelineLayout weightedColorPipelineLayout;
	VkPipeline weightedColorPipeline;
	VkPipelineLayout weightedRevealPipelineLayout;
	VkPipeline weightedRevealPipeline;
	//Pipeline weightedColorPipeline;
	//Pipeline weightedRevealPipeline;

	// UI
	RenderPass uiRenderPass;
	std::vector<VkFramebuffer> uiFramebuffers;

	uint32_t currentFrame;

	void initVulkan();

	void cleanUpVulkan();

	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	void setupDebugMessenger();

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	
	void createInstance();

	bool checkValidationLayerSupport();

	// The validation layers will print debug messages to the standard output by default, 
	// but we can also handle them ourselves by providing an explicit callback in our program. 
	// This will also allow you to decide which kind of messages you would like to see, 
	// because not all are necessarily (fatal) errors.
	std::vector<const char*> getRequiredExtensions();

	void createSurface();
	
	void pickPhysicalDevice();

	int rateDeviceSuitability(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	void createLogicalDevice();

	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void createSwapChain();

	void createSwapchainImageViews();

	VkShaderModule createShaderModule(const std::vector<char>& code);

	void createSwapchainFramebuffers();

	void createCommandPool();

	void createCommandBuffers();

	VkCommandBuffer beginSingleTimeCommands();

	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void createSyncObjects();

	void drawFrame(ImGuiIO& io);

	void cleanupSwapChain();

	void recreateSwapChain();

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);	

	MeshBuffer createVertexBuffer(std::vector<Vertex> vertices);
	
	MeshBuffer createIndexBuffer(std::vector<uint32_t> indices);

	void createVertexAndIndexBuffers();

	void createUniformBuffers();

	void updateUniformBuffer(uint32_t currentImage);

	void createDescriptor();

	VulkanImage createTextureImage(int texWidth, int texHeight, stbi_uc* pixels);

	void createTextureImages();

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layers = 1);

	void createSampler(VkSampler* sampler, float mipLevels, bool useNearestFilter = false, bool isEnvMap = false);

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	VkFormat findDepthFormat();

	bool hasStencilComponent(VkFormat format);

	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	VkSampleCountFlagBits getMaxUsableSampleCount();

	// Functions added for IBL
	VulkanImage createTextureImageGeneric(
		int texWidth,
		int  texHeight,
		const void* pixels,
		uint32_t* mipLevels,
		VkFormat format,
		size_t bytesPerChannel /* 1 or 4 */);

	void createFlattenedEnvironmentMapImage(CubeMap flattenedEnvMap, bool useHigherPrecision = false);

	// HDR (32-bit)
	void createEnvMapImage(const HDRImage& envMap);

	// Functions added for BWOIT
	void recordDrawForMesh(VkCommandBuffer cmd, const std::string& name);

	void createFramebuffers();

	void createImageResource(VulkanImage* image, VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags);

	void createOffscreenImageResources();

	void transitionImage(
		VulkanImage image, 
		VkImageLayout newLayout, 
		VkAccessFlags newtAccesses, 
		VkImageSubresourceRange range = { 0 }
	);

	void createRenderPasses();

	void createOpaqueObjectsFramebuffer();

	void createTransparentObjectsFramebuffer();

	void createOpaqueObjectsPipeline(std::vector<char>vertShaderCode, std::vector<char>fragShaderCode);

	void createWeightedColorPipeline(std::vector<char>vertShaderCode, std::vector<char>fragShaderCode);

	void createWeightedRevealPipeline(std::vector<char>vertShaderCode, std::vector<char>fragShaderCode);

	void createPipelines();

	void recordOpaqueObjectsRenderPass(VkCommandBuffer commandBuffer);

	void recordTransparentObjectsRenderPass(VkCommandBuffer commandBuffer);

	void recordSwapchainBlit(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void createUIFramebuffers();

	void createUI();

	void recordUIRenderPass(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};