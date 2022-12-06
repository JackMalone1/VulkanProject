#pragma once

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices {
	int graphicsFamily = -1;	
	int presentationFamily = -1;

	// Check if queue families are valid
	bool isValid()
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;	
	std::vector<VkSurfaceFormatKHR> formats;		
	std::vector<VkPresentModeKHR> presentationModes;
};