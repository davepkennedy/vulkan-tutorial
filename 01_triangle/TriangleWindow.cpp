#include "TriangleWindow.h"

#include "Application.h"
#include <vulkan\vulkan_win32.h>
#include <vector>
#include <set> 
#include <fstream>
#include <string>

#include <cstring>

DECLARE_APP(TriangleWindow)

const int WIDTH = 800;
const int HEIGHT = 600;

const int TriangleWindow::MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData) 
{
	OutputDebugStringA("validation layer: ");
	OutputDebugStringA(msg);
	OutputDebugStringA("\n");

	return VK_FALSE;
}

VkResult CreateDebugReportCallbackEXT(
	VkInstance instance,
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugReportCallbackEXT(
	VkInstance instance,
	VkDebugReportCallbackEXT callback, 
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

TriangleWindow::TriangleWindow()
	: currentFrame(0)
{
	observe(WM_CREATE, [this](WPARAM wParam, LPARAM lParam) {
		Size(WIDTH, HEIGHT);
		initVulkan();
	});

	observe(WM_SIZE, [this](WPARAM wParam, LPARAM lParam) {
		recreateSwapChain();
	});

	observe(WM_DESTROY, [this](WPARAM wParam, LPARAM lParam) {
		cleanupSwapChain();

		for (auto semaphore : imageAvailableSemaphores) {
			device.destroySemaphore(semaphore);
		}
		for (auto semaphore : renderFinishedSemaphores) {
			device.destroySemaphore(semaphore);
		}
		for (auto fence : inFlightFences) {
			device.destroyFence(fence);
		}

		device.destroyCommandPool(commandPool);
		device.destroy();
		instance.destroySurfaceKHR(surface);
		if (enableValidationLayers) {
			DestroyDebugReportCallbackEXT((VkInstance)instance, callback, nullptr);
		}
		instance.destroy();
	});
}

void TriangleWindow::recreateSwapChain()
{
	if (device) {
		device.waitIdle();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
	}
}

void TriangleWindow::cleanupSwapChain()
{
	for (auto buffer : swapChainFramebuffers) {
		device.destroyFramebuffer(buffer);
	}
	device.freeCommandBuffers(commandPool, commandBuffers);
	device.destroyPipeline(graphicsPipeline);
	device.destroyPipelineLayout(pipelineLayout);
	device.destroyRenderPass(renderPass);

	for (auto swapChainImageView : swapChainImageViews) {
		device.destroyImageView(swapChainImageView);
	}
	device.destroySwapchainKHR(swapChain);
}

TriangleWindow::~TriangleWindow()
{
}

void TriangleWindow::initVulkan()
{
	OutputDebugString(TEXT("InitVulkan\n"));

	createInstance();
	setupDebugCallback();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline(); 
	createFramebuffers();
	createCommandPool();
	createCommandBuffers();
	createSyncObjects();
}

std::vector<const char*> TriangleWindow::getRequiredExtensions() {
	std::vector<const char*> extensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	};

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	return extensions;
}

bool TriangleWindow::checkValidationLayerSupport() {
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

void TriangleWindow::createInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	vk::ApplicationInfo appInfo = vk::ApplicationInfo()
		.setPApplicationName("Hello Triangle")
		.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
		.setPEngineName("No Engine")
		.setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
		.setApiVersion(VK_API_VERSION_1_0);

	std::vector<const char*> extensions = getRequiredExtensions();

	auto extensionProperties = vk::enumerateInstanceExtensionProperties();
	for (const auto& prop : extensionProperties)
	{
		OutputDebugStringA(prop.extensionName);
		OutputDebugStringA("\n");
	}
	vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
		.setPApplicationInfo(&appInfo)
		.setEnabledExtensionCount(extensions.size())
		.setPpEnabledExtensionNames(extensions.data())
		.setEnabledLayerCount(enableValidationLayers ? validationLayers.size() : 0)
		.setPpEnabledLayerNames(enableValidationLayers ? validationLayers.data() : nullptr);

	instance = vk::createInstance(createInfo);
}

void TriangleWindow::setupDebugCallback()
{
	if (!enableValidationLayers) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_DEBUG_BIT_EXT;
	createInfo.pfnCallback = debugCallback;


	if (CreateDebugReportCallbackEXT((VkInstance)instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}

void TriangleWindow::pickPhysicalDevice()
{ 
	physicalDevice = optimalDevice();
}

bool TriangleWindow::isDeviceSuitable(const vk::PhysicalDevice& device) const
{
	auto deviceProperties = device.getProperties();
	auto deviceFeatures = device.getFeatures();

	OutputDebugStringA("Device: ");
	OutputDebugStringA(deviceProperties.deviceName);

	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return findQueueFamilies(device).isComplete() && extensionsSupported && swapChainAdequate;
}

vk::PhysicalDevice TriangleWindow::optimalDevice() const 
{
	for (const auto& device : instance.enumeratePhysicalDevices()) {
		if (isDeviceSuitable(device)) {
			return device;
		}
	}
	throw std::runtime_error("failed to find a suitable GPU!");
}

bool TriangleWindow::checkDeviceExtensionSupport(vk::PhysicalDevice device) const {
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : device.enumerateDeviceExtensionProperties()) {
		requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}

QueueFamilyIndices TriangleWindow::findQueueFamilies(const vk::PhysicalDevice& device) const
{
	QueueFamilyIndices indices;
	int i = 0;
	for (const auto& queueFamily : device.getQueueFamilyProperties()) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}
		VkBool32 presentSupport = device.getSurfaceSupportKHR(i, surface);
		if (queueFamily.queueCount > 0 && presentSupport) {
			indices.presentFamily = i;
		}
		if (indices.isComplete()) {
			break;
		}
		i++;
	}

	return indices;
}

void TriangleWindow::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };
	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo()
			.setQueueFamilyIndex(queueFamily)
			.setQueueCount(1)
			.setPQueuePriorities(&queuePriority);
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures();

	vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
		.setPQueueCreateInfos(queueCreateInfos.data())
		.setQueueCreateInfoCount(queueCreateInfos.size())
		.setPEnabledFeatures(&deviceFeatures)
		.setPpEnabledExtensionNames(deviceExtensions.data())
		.setEnabledExtensionCount(deviceExtensions.size())
		.setEnabledLayerCount(enableValidationLayers ? validationLayers.size() : 0)
		.setPpEnabledLayerNames(enableValidationLayers ? validationLayers.data() : nullptr);

	device = physicalDevice.createDevice(createInfo);

	graphicsQueue = device.getQueue(indices.graphicsFamily, 0);
	presentQueue = device.getQueue(indices.presentFamily, 0);
}

void TriangleWindow::createSurface()
{
	vk::Win32SurfaceCreateInfoKHR createInfo = vk::Win32SurfaceCreateInfoKHR()
		.setHwnd(Handle())
		.setHinstance(GetModuleHandle(nullptr));
	surface = instance.createWin32SurfaceKHR(createInfo);
}


SwapChainSupportDetails TriangleWindow::querySwapChainSupport(const vk::PhysicalDevice& device) const
{
	SwapChainSupportDetails details;
	details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
	details.formats = device.getSurfaceFormatsKHR(surface);
	details.presentModes = device.getSurfacePresentModesKHR(surface);
	return details;
}

vk::SurfaceFormatKHR TriangleWindow::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const
{
	if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
		return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	}

	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

vk::PresentModeKHR TriangleWindow::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const
{
	vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;
	for (const auto& availableMode : availablePresentModes) {
		if (availableMode == vk::PresentModeKHR::eMailbox) {
			return availableMode;
		}
		else if (availableMode == vk::PresentModeKHR::eImmediate) {
			bestMode = availableMode;
		}
	}
	return bestMode;
}

vk::Extent2D TriangleWindow::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		vk::Extent2D actualExtent = { (uint32_t)width(), (uint32_t)height() };
		actualExtent.width = std::max(capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void TriangleWindow::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
	vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	vk::SwapchainCreateInfoKHR createInfo = vk::SwapchainCreateInfoKHR()
		.setSurface(surface)
		.setMinImageCount(imageCount)
		.setImageFormat(surfaceFormat.format)
		.setImageColorSpace(surfaceFormat.colorSpace)
		.setImageExtent(extent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
			.setQueueFamilyIndexCount(2)
			.setPQueueFamilyIndices(queueFamilyIndices);
	}
	else {
		createInfo.setImageSharingMode(vk::SharingMode::eExclusive)
			.setQueueFamilyIndexCount(0) // Optional
			.setPQueueFamilyIndices(nullptr); // Optional
	}
	createInfo.setPreTransform(swapChainSupport.capabilities.currentTransform)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(presentMode)
		.setClipped(VK_TRUE)
		.setOldSwapchain(VK_NULL_HANDLE);

	swapChain = device.createSwapchainKHR(createInfo);

	swapChainImages = device.getSwapchainImagesKHR(swapChain);

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void TriangleWindow::createImageViews()
{
	swapChainImageViews.erase(swapChainImageViews.begin(), swapChainImageViews.end());
	for (const auto& swapChainImage : swapChainImages) {
		vk::ImageViewCreateInfo createInfo = vk::ImageViewCreateInfo()
			.setImage(swapChainImage)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(swapChainImageFormat)
			.setComponents({ vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity })
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
		swapChainImageViews.push_back(device.createImageView(createInfo));
	}
}

void TriangleWindow::createRenderPass()
{
	vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
		.setFormat(swapChainImageFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	vk::AttachmentReference colorAttachmentRef = vk::AttachmentReference()
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription subpass = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(1)
		.setPColorAttachments(&colorAttachmentRef);

	vk::SubpassDependency dependency = vk::SubpassDependency()
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		//.setSrcAccessMask(0)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);


	vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(1)
		.setPAttachments(&colorAttachment)
		.setDependencyCount(1)
		.setPDependencies(&dependency)
		.setSubpassCount(1)
		.setPSubpasses(&subpass);

	renderPass = device.createRenderPass(renderPassInfo);
}

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

void TriangleWindow::createGraphicsPipeline() {
	auto vertShaderCode = readFile("vert.spv");
	auto fragShaderCode = readFile("frag.spv");

	vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(vertShaderModule)
		.setPName("main");

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(fragShaderModule)
		.setPName("main");
	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptionCount(0)
		.setPVertexBindingDescriptions(nullptr) // Optional
		.setVertexAttributeDescriptionCount(0)
		.setPVertexAttributeDescriptions(nullptr);  // Optional

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList)
		.setPrimitiveRestartEnable(VK_FALSE);

	vk::Viewport viewport = vk::Viewport()
		.setX(0)
		.setY(0)
		.setWidth(swapChainExtent.width)
		.setHeight(swapChainExtent.height)
		.setMinDepth(0)
		.setMaxDepth(1);

	vk::Rect2D scissor = vk::Rect2D({ 0,0 }, swapChainExtent);

	vk::PipelineViewportStateCreateInfo viewportState = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setPViewports(&viewport)
		.setScissorCount(1)
		.setPScissors(&scissor);

	vk::PipelineRasterizationStateCreateInfo rasterizer = vk::PipelineRasterizationStateCreateInfo()
		.setDepthClampEnable(VK_FALSE)
		.setRasterizerDiscardEnable(VK_FALSE)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setLineWidth(1)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eClockwise)
		.setDepthBiasEnable(VK_FALSE)
		.setDepthBiasConstantFactor(0)
		.setDepthBiasClamp(0)
		.setDepthBiasSlopeFactor(0);

	vk::PipelineMultisampleStateCreateInfo multisampling = vk::PipelineMultisampleStateCreateInfo()
		.setSampleShadingEnable(VK_FALSE)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setMinSampleShading(1)
		.setPSampleMask(nullptr)
		.setAlphaToCoverageEnable(VK_FALSE)
		.setAlphaToOneEnable(VK_FALSE);

	vk::PipelineColorBlendAttachmentState colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
		.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
		.setBlendEnable(VK_FALSE)
		.setSrcColorBlendFactor(vk::BlendFactor::eOne)
		.setDstColorBlendFactor(vk::BlendFactor::eZero)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		.setAlphaBlendOp(vk::BlendOp::eAdd);

	vk::PipelineColorBlendStateCreateInfo colorBlending = vk::PipelineColorBlendStateCreateInfo()
		.setLogicOpEnable(VK_FALSE)
		.setLogicOp(vk::LogicOp::eCopy)
		.setAttachmentCount(1)
		.setPAttachments(&colorBlendAttachment)
		.setBlendConstants({ 0.0f,0.0f,0.0f,0.0f });

	vk::DynamicState dynamicStates[] = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eLineWidth
	};
	vk::PipelineDynamicStateCreateInfo dyanmicState = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStateCount(2)
		.setPDynamicStates(dynamicStates);

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount(0)
		.setPSetLayouts(nullptr)
		.setPushConstantRangeCount(0)
		.setPPushConstantRanges(nullptr);

	pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

	vk::GraphicsPipelineCreateInfo pipelineInfo = vk::GraphicsPipelineCreateInfo()
		.setStageCount(2)
		.setPStages(shaderStages)
		.setPVertexInputState(&vertexInputInfo)
		.setPInputAssemblyState(&inputAssembly)
		.setPViewportState(&viewportState)
		.setPRasterizationState(&rasterizer)
		.setPMultisampleState(&multisampling)
		.setPDepthStencilState(nullptr)
		.setPColorBlendState(&colorBlending)
		.setPDynamicState(nullptr)
		.setLayout(pipelineLayout)
		.setRenderPass(renderPass)
		.setSubpass(0)
		.setBasePipelineHandle(VK_NULL_HANDLE)
		.setBasePipelineIndex(-1);

	graphicsPipeline = device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);

	device.destroyShaderModule(vertShaderModule);
	device.destroyShaderModule(fragShaderModule);
}

vk::ShaderModule TriangleWindow::createShaderModule(const std::vector<char>& code) {
	vk::ShaderModuleCreateInfo createInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(code.size())
		.setPCode(reinterpret_cast<const uint32_t*>(code.data()));
	return device.createShaderModule(createInfo);
}

void TriangleWindow::createFramebuffers()
{
	swapChainFramebuffers.erase(swapChainFramebuffers.begin(), swapChainFramebuffers.end());
	for (const auto& swapChainImageView : swapChainImageViews) {
		vk::ImageView attachments[] = {
			swapChainImageView
		};

		vk::FramebufferCreateInfo frameBufferInfo = vk::FramebufferCreateInfo()
			.setRenderPass(renderPass)
			.setAttachmentCount(1)
			.setPAttachments(attachments)
			.setWidth(swapChainExtent.width)
			.setHeight(swapChainExtent.height)
			.setLayers(1);

		swapChainFramebuffers.push_back(device.createFramebuffer(frameBufferInfo));
	}
}

void TriangleWindow::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
	vk::CommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo()
		.setQueueFamilyIndex(queueFamilyIndices.graphicsFamily);
		//.setFlags(0);
	commandPool = device.createCommandPool(poolInfo);
}

void TriangleWindow::createCommandBuffers()
{
	vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
		.setCommandPool(commandPool)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(swapChainFramebuffers.size());
	commandBuffers = device.allocateCommandBuffers(allocInfo);

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
			.setPInheritanceInfo(nullptr);
		commandBuffers[i].begin(beginInfo);

		std::array<float, 4> colorComponents = { 0.0f,0.0f,0.0f,0.0f };
		vk::ClearColorValue clearColor = vk::ClearColorValue(colorComponents);
		vk::ClearValue clearValue = vk::ClearValue(clearColor);
		vk::RenderPassBeginInfo renderPassInfo = vk::RenderPassBeginInfo()
			.setRenderPass(renderPass)
			.setFramebuffer(swapChainFramebuffers[i])
			.setRenderArea(vk::Rect2D({ 0,0 }, swapChainExtent))
			.setClearValueCount(1)
			.setPClearValues(&clearValue);
		commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
		commandBuffers[i].draw(3, 1, 0, 0);
		commandBuffers[i].endRenderPass();
		commandBuffers[i].end();
	}
}

void TriangleWindow::createSyncObjects()
{
	vk::SemaphoreCreateInfo semaphoreInfo;
	vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo()
		.setFlags(vk::FenceCreateFlagBits::eSignaled);
	imageAvailableSemaphores.erase(imageAvailableSemaphores.begin(), imageAvailableSemaphores.end());
	renderFinishedSemaphores.erase(renderFinishedSemaphores.begin(), renderFinishedSemaphores.end());
	inFlightFences.erase(inFlightFences.begin(), inFlightFences.end());

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		imageAvailableSemaphores.push_back(device.createSemaphore(semaphoreInfo));
		renderFinishedSemaphores.push_back(device.createSemaphore(semaphoreInfo));
		inFlightFences.push_back(device.createFence(fenceInfo));
	}
}

void TriangleWindow::drawFrame()
{
	if (commandBuffers.size() == 0) {
		return;
	}
	device.waitForFences({ inFlightFences[currentFrame] }, VK_TRUE, std::numeric_limits<uint64_t>::max());
	device.resetFences({ inFlightFences[currentFrame] });
	auto result = device.acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE);
	if (result.result == vk::Result::eErrorOutOfDateKHR) {
		recreateSwapChain();
		return;
	}
	uint32_t imageIndex = result.value;

	vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	vk::PipelineStageFlags waitStages[] = {
		vk::PipelineStageFlagBits::eColorAttachmentOutput
	};
	vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	vk::SubmitInfo submitInfo = vk::SubmitInfo()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(waitSemaphores)
		.setPWaitDstStageMask(waitStages)
		.setCommandBufferCount(1)
		.setPCommandBuffers(&commandBuffers[imageIndex])
		.setSignalSemaphoreCount(1)
		.setPSignalSemaphores(signalSemaphores);

	graphicsQueue.submit({ submitInfo }, inFlightFences[currentFrame]);

	vk::SwapchainKHR swapChains[] = { swapChain };
	vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(signalSemaphores)
		.setSwapchainCount(1)
		.setPSwapchains(swapChains)
		.setPImageIndices(&imageIndex)
		.setPResults(nullptr);
	presentQueue.presentKHR(presentInfo);
	presentQueue.waitIdle();

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}