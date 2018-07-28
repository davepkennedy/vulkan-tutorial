#pragma once

#include <vulkan/vk_sdk_platform.h>
#include "Window.h"

#include <vulkan\vulkan.hpp>
#include <glm\glm.hpp>

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	static inline vk::VertexInputBindingDescription getBindingDescription() {
		vk::VertexInputBindingDescription bindingDescription = vk::VertexInputBindingDescription()
			.setBinding(0)
			.setStride(sizeof(Vertex))
			.setInputRate(vk::VertexInputRate::eVertex);

		return bindingDescription;
	}

	static inline std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;
		attributeDescriptions[0].setBinding(0)
			.setLocation(0)
			.setFormat(vk::Format::eR32G32Sfloat)
			.setOffset(offsetof(Vertex, pos));
		attributeDescriptions[1].setBinding(0)
			.setLocation(1)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(Vertex, pos));
		return attributeDescriptions;
	}
};

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool isComplete() {
		return (graphicsFamily >= 0 && presentFamily >= 0);
	}
};

struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

class VertexBufferWindow :
	public Window
{
private:
	vk::Instance instance;

	VkDebugReportCallbackEXT callback;

	vk::PhysicalDevice physicalDevice;
	vk::Device device;

	vk::Queue graphicsQueue;
	vk::Queue presentQueue;

	vk::SurfaceKHR surface;

	vk::SwapchainKHR swapChain;
	std::vector<vk::Image> swapChainImages;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;

	std::vector<vk::ImageView> swapChainImageViews;

	vk::RenderPass renderPass;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;

	std::vector<vk::Framebuffer> swapChainFramebuffers;

	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;

	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	size_t currentFrame;

	vk::Buffer vertexBuffer;
	vk::DeviceMemory vertexBufferMemory;

	vk::Buffer indexBuffer;
	vk::DeviceMemory indexBufferMemory;

	static const int MAX_FRAMES_IN_FLIGHT;
	static const std::vector<Vertex> vertices;
	static const std::vector<uint16_t> indices;
protected:
	void initVulkan();
	void createInstance();
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	void setupDebugCallback();
	void pickPhysicalDevice();
	bool isDeviceSuitable(const vk::PhysicalDevice& device) const;
	bool checkDeviceExtensionSupport(vk::PhysicalDevice device) const;
	vk::PhysicalDevice optimalDevice() const;

	QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& device) const;

	void createLogicalDevice();

	void createSurface();

	SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice& device) const;

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const;
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createGraphicsPipeline();
	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	void createFramebuffers();
	void createCommandPool();
	void createVertexBuffers();
	void createIndexBuffers();
	void createCommandBuffers();
	void createSyncObjects();

	void recreateSwapChain();
	void cleanupSwapChain();

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

	void createBuffer(
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags properties,
		vk::Buffer& buffer,
		vk::DeviceMemory& bufferMemory);
	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

public:
	VertexBufferWindow();
	~VertexBufferWindow();

	void drawFrame();
};

