#include "VulkanRenderer.h"


VulkanRenderer::VulkanRenderer()
{
}

int VulkanRenderer::init(GLFWwindow* newWindow)
{
	window = newWindow;

	try
	{
		createInstance();
		createDebugCallback();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
	}
	catch (const std::runtime_error& e)
	{
		VULKAN_CORE_ERROR(e.what());
		return EXIT_FAILURE;
	}

	return 0;
}

void VulkanRenderer::cleanup()
{
	vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	if (validationEnabled)
	{
		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	}
	vkDestroyInstance(instance, nullptr);
}


VulkanRenderer::~VulkanRenderer()
{
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
	create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = instanceCreationDestructionDebugCallback;
}

void VulkanRenderer::create_app_info(VkApplicationInfo& appInfo)
{
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
}

void VulkanRenderer::createInstance()
{
	if (validationEnabled && !checkValidationLayerSupport())
	{
		throw std::runtime_error("Required Validation Layers not supported!");
	}

	VkApplicationInfo appInfo = {};
	create_app_info(appInfo);

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto instanceExtensions = std::vector<const char*>();

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	if (validationEnabled)
	{
		instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	if (!checkInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (validationEnabled)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = &debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan Instance!");
	}
}

void VulkanRenderer::createDebugCallback()
{
	if constexpr (!validationEnabled) return;

	VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {};
	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	callbackCreateInfo.pfnCallback = debugCallback;

	VkResult result = CreateDebugReportCallbackEXT(instance, &callbackCreateInfo, nullptr, &callback);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Debug Callback!");
	}
	VULKAN_CORE_INFO("Succesfully created debug callback");
}

void VulkanRenderer::createLogicalDevice()
{
	const QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = {indices.graphicsFamily, indices.presentationFamily};

	for (int queueFamilyIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	VkPhysicalDeviceFeatures deviceFeatures = {};

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Logical Device!");
	}

	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}

void VulkanRenderer::createSurface()
{
	const VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a surface!");
	}
}

void VulkanRenderer::createSwapChain()
{
	SwapChainDetails swap_chain_details = getSwapChainDetails(mainDevice.physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swap_chain_details.formats);
	VkPresentModeKHR presentMode = chooseBestPresentationMode(swap_chain_details.presentationModes);
	VkExtent2D extent = chooseSwapExtent(swap_chain_details.surfaceCapabilities);

	uint32_t imageCount = swap_chain_details.surfaceCapabilities.minImageCount + 1;

	if (swap_chain_details.surfaceCapabilities.maxImageCount > 0 && 
		swap_chain_details.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swap_chain_details.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform = swap_chain_details.surfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.clipped = VK_TRUE;

	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);
	if (indices.graphicsFamily != indices.presentationFamily)
	{
		uint32_t queueFamilyIndices[] = {
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentationFamily
		};
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &swapchain);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a swapchain");
	}
}

void VulkanRenderer::getPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("Can't find GPUs that support Vulkan Instance");
	}

	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

	for (const auto& device : deviceList)
	{
		if (checkDeviceSuitable(device))
		{
			mainDevice.physicalDevice = device;
			VULKAN_CORE_INFO("Found a suitable physical device to use");
			break;
		}
	}
}

bool VulkanRenderer::check_extension_support(std::vector<VkExtensionProperties> extensions,
                                             const std::vector<const char*>::value_type& checkExtension)
{
	bool hasExtension = false;
	for (const auto& extension : extensions)
	{
		if (strcmp(checkExtension, extension.extensionName) == 0)
		{
			hasExtension = true;
			VULKAN_CORE_TRACE("Can support given extension");
			break;
		}
	}

	if (!hasExtension)
	{
		VULKAN_CORE_WARN("Can't support given extension");
		return false;
	}
	return true;
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	for (const auto& checkExtension : *checkExtensions)
	{
		if (!check_extension_support(extensions, checkExtension)) return false;
	}

	return true;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	if (extensionCount == 0)
	{
		VULKAN_CORE_WARN("No extensions found that the physical device can support");
		return false;
	}

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	// Check for extension
	for (const auto& deviceExtension : deviceExtensions)
	{
		if (!check_extension_support(extensions, deviceExtension)) return false;
	}

	return true;
}

bool VulkanRenderer::checkValidationLayerSupport()
{
	uint32_t validationLayerCount;
	vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);

	if (validationLayerCount == 0 && !validationLayers.empty())
	{
		return false;
	}

	std::vector<VkLayerProperties> availableLayers(validationLayerCount);
	vkEnumerateInstanceLayerProperties(&validationLayerCount, availableLayers.data());

	for (const auto& validationLayer : validationLayers)
	{
		bool hasLayer = false;
		for (const auto& availableLayer : availableLayers)
		{
			if (strcmp(validationLayer, availableLayer.layerName) == 0)
			{
				hasLayer = true;
				break;
			}
		}

		if (!hasLayer)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = getQueueFamilies(device);

	const bool extensions_supported = checkDeviceExtensionSupport(device);

	bool swap_chain_valid = false;
	if (extensions_supported)
	{
		const SwapChainDetails swap_chain_details = getSwapChainDetails(device);
		swap_chain_valid = !swap_chain_details.presentationModes.empty() && !swap_chain_details.formats.empty();
	}

	return indices.isValid() && extensions_supported && swap_chain_valid;
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilyList)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);

		if (queueFamily.queueCount > 0 && presentationSupport)
		{
			indices.presentationFamily = i;
		}

		if (indices.isValid())
		{
			break;
		}

		i++;
	}

	return indices;
}

SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
{
	SwapChainDetails swapChainDetails;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
	}

	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

	if (presentationCount != 0)
	{
		swapChainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount,
		                                          swapChainDetails.presentationModes.data());
	}

	return swapChainDetails;
}

VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& format : formats)
	{
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
			&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	for (const auto& presentationMode : presentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;//this must always exist
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCabalities)
{
	if (surfaceCabalities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCabalities.currentExtent;
	}

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	VkExtent2D newExtent = {};
	newExtent.width = static_cast<uint32_t>(width);
	newExtent.height = static_cast<uint32_t>(height);
	newExtent.width = std::max(surfaceCabalities.minImageExtent.width,
		std::min(surfaceCabalities.maxImageExtent.width, newExtent.width));
	newExtent.height = std::max(surfaceCabalities.minImageExtent.height,
		std::min(surfaceCabalities.maxImageExtent.height, newExtent.height));

	return newExtent;
}
