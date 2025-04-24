#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <set>
#include <map>
#include <optional>
#include <cstdint> 
#include <limits> 
#include <algorithm> 
#include <unordered_map>

#include "camera.h"
#include "uniformBuffer.h"
#include "vertex.h"
#include "vulkanImage.h"
#include "common.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const int MAX_FRAMES_IN_FLIGHT = 2;

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
		std::unordered_map<std::string, std::vector<char>> shaders,
		std::unordered_map<std::string, std::pair<std::vector<Vertex>, std::vector<uint32_t>>> models,
		std::unordered_map<std::string, Image> textures);

	~Main();

	void mainLoop();

private:
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

	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;

	VkDescriptorSetLayout descriptorSetLayout;

	std::unordered_map<std::string, std::vector<char>> shaders;
	std::unordered_map<std::string, std::pair<std::vector<Vertex>, std::vector<uint32_t>>> models;
	std::unordered_map<std::string, Image> textures;

	std::unordered_map<std::string, MeshBuffer> vertices;
	std::unordered_map<std::string, MeshBuffer> indices;

	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
	// Command buffers will be automatically freed when their command pool is destroyed, so we don't need explicit cleanup.
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	// The images were created by the implementation for the swap chain and they will be automatically cleaned up once the swap chain has been destroyed
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	uint32_t mipLevels;
	VkSampler textureSampler;
	VkSampleCountFlagBits msaaSamples;

	std::vector<VulkanImage> swapChainImageViews;
	std::unordered_map<std::string, VulkanImage> textureImages;
	VulkanImage colorImage;
	VulkanImage depthImage;
	VulkanImage offscreenColorImage;
	VulkanImage weightedColorImage;
	VulkanImage weightedRevealImage;
	// Given that we use multisampling for images before this step, we need to downsample it before blitting the result to the swapchain
	VulkanImage swapChainImage;

	// Opaque Objects
	VkRenderPass opaqueObjectsRenderPass;
	VkFramebuffer opaqueObjectsFramebuffer;
	VkPipelineLayout opaqueObjectsPipelineLayout;
	VkPipeline opaqueObjectsPipeline;
	// Transparent Objects
	// TODO: remember to delete this when used
	VkRenderPass transparentObjectsRenderPass;
	VkFramebuffer transparentObjectsFramebuffer;
	VkPipelineLayout weightedColorPipelineLayout;
	VkPipeline weightedColorPipeline;
	VkPipelineLayout weightedRevealPipelineLayout;
	VkPipeline weightedRevealPipeline;

	uint32_t currentFrame;

	void initVulkan();

	void cleanUp();

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

	void createDescriptorSetLayout();

	void createSwapchainFramebuffers();

	void createCommandPool();

	void createCommandBuffers();

	VkCommandBuffer beginSingleTimeCommands();

	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void createSyncObjects();

	void drawFrame();

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

	void createDescriptorPool();

	void createDescriptorSets();

	VulkanImage createTextureImage(int texWidth, int texHeight, stbi_uc* pixels);

	void createTextureImages();

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createTextureSampler();

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	VkFormat findDepthFormat();

	bool hasStencilComponent(VkFormat format);

	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	VkSampleCountFlagBits getMaxUsableSampleCount();

	// Functions added for BWOIT
	void createFramebuffers();

	void createImageResource(VulkanImage* image, VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags);

	void createImageResources();

	void transitionImage(VulkanImage image, VkImageLayout newLayout, VkAccessFlags newtAccesses);

	void createOpaqueObjectsFramebuffer();

	void createOpaqueObjectsRenderPass();

	void createOpaqueObjectsPipeline(std::vector<char>vertShaderCode, std::vector<char>fragShaderCode);

	void recordOpaqueObjectsRenderPass(VkCommandBuffer commandBuffer);

	void createTransparentObjectsRenderPass();

	void createTransparentObjectsFramebuffer();

	void createWeightedColorPipeline(std::vector<char>vertShaderCode, std::vector<char>fragShaderCode);

	void createWeightedRevealPipeline(std::vector<char>vertShaderCode, std::vector<char>fragShaderCode);

	void recordTransparentObjectsRenderPass(VkCommandBuffer commandBuffer);

	void recordSwapchainBlit(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};