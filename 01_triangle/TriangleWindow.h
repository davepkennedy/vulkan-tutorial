#pragma once

#include <vulkan/vk_sdk_platform.h>
#include "Window.h"

#include <vulkan\vulkan.hpp>

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

class TriangleWindow :
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

	static const int MAX_FRAMES_IN_FLIGHT;
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
	void createCommandBuffers();
	void createSyncObjects();

	void recreateSwapChain();
	void cleanupSwapChain();
public:
	TriangleWindow();
	~TriangleWindow();

	void drawFrame();
};

