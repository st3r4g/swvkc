#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <vulkan/vulkan.h>

VkInstance create_instance() {
	const char *extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_DISPLAY_EXTENSION_NAME,
	};
	VkInstanceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.enabledExtensionCount = sizeof(extensions)/sizeof(char*),
		.ppEnabledExtensionNames = extensions
	};
	VkInstance inst;
	if (vkCreateInstance(&info, NULL, &inst)) {
		fprintf(stderr, "ERROR: create_instance() failed.\n");
		return VK_NULL_HANDLE;
	}
	return inst;
}

VkPhysicalDevice get_physical_device(VkInstance inst) {
	uint32_t n = 1;
	VkPhysicalDevice pdev;
	vkEnumeratePhysicalDevices(inst, &n, &pdev);
	if (n == 0) {
		fprintf(stderr, "ERROR: get_physical_device() failed.\n");
		return VK_NULL_HANDLE;
	}
	return pdev;
}

VkDevice create_device(VkInstance inst, VkPhysicalDevice pdev) {
	const char *extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
		VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
	};
	float priority = 1.0f;
	VkDeviceQueueCreateInfo infoQueue = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};
	VkDeviceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &infoQueue,
		.enabledExtensionCount = sizeof(extensions)/sizeof(char*),
		.ppEnabledExtensionNames = extensions,
	};
	VkDevice dev;
	if (vkCreateDevice(pdev, &info, NULL, &dev)) {
		fprintf(stderr, "ERROR: create_device() failed.\n");
		return VK_NULL_HANDLE;
	}
	return dev;
}

VkBuffer create_buffer(VkDevice dev) {
	VkExternalMemoryBufferCreateInfo info_next = {
		.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO,
		.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
	};
	VkBufferCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = &info_next,
		// TODO
	};
	VkBuffer buf;
	if (vkCreateBuffer(dev, &info, NULL, &buf)) {
		fprintf(stderr, "ERROR: create_buffer() failed.\n");
		return VK_NULL_HANDLE;
	}
	return buf;
}

VkDeviceMemory import_memory(int fd, VkDevice dev) {
	VkImportMemoryFdInfoKHR info_next = {
		.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
		.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
// or maybe VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
		.fd = fd,
	};
	VkMemoryAllocateInfo info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &info_next
		// TODO
	};
	VkDeviceMemory mem;
	if (vkAllocateMemory(dev, &info, NULL, &mem)) {
		fprintf(stderr, "ERROR: import_memory() failed.\n");
		return VK_NULL_HANDLE;
	}
	return mem;
}

VkResult bind_buffer_memory(VkDevice dev, VkBuffer buf, VkDeviceMemory mem) {
	return vkBindBufferMemory(dev, buf, mem, 0);
}

VkSurfaceKHR create_surface(VkInstance inst, VkPhysicalDevice pdev) {
	uint32_t n = 1;
	VkDisplayPropertiesKHR propsDisplay;
	vkGetPhysicalDeviceDisplayPropertiesKHR(pdev, &n, &propsDisplay);
	if (n == 0) {
		fprintf(stderr, "ERROR: create_surface() failed.\n");
		return VK_NULL_HANDLE;
	}

	VkDisplayModePropertiesKHR props;
	vkGetDisplayModePropertiesKHR(pdev, propsDisplay.display, &n, &props);

	VkDisplaySurfaceCreateInfoKHR info = {
		.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR,
		.displayMode = props.displayMode,
		.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR,
		.imageExtent = props.parameters.visibleRegion
	};
	VkSurfaceKHR surf;
	if (vkCreateDisplayPlaneSurfaceKHR(inst, &info, NULL, &surf)) {
		fprintf(stderr, "ERROR: create_surface() failed.\n");
		return VK_NULL_HANDLE;
	}
	return surf;
}

VkSwapchainKHR create_swapchain(VkPhysicalDevice pdev, VkDevice dev, VkSurfaceKHR surf) {
	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surf, &caps);

	VkSwapchainCreateInfoKHR info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surf,
		.minImageCount = 1,
		.imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = caps.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};
	VkSwapchainKHR swp;
	if (vkCreateSwapchainKHR(dev, &info, NULL, &swp)) {
		fprintf(stderr, "ERROR: create_swapchain() failed.\n");
		return VK_NULL_HANDLE;
	}
	return swp;
}

VkCommandPool create_command_pool(VkDevice dev) {
	VkCommandPoolCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	};
	VkCommandPool pool;
	if (vkCreateCommandPool(dev, &info, NULL, &pool)) {
		fprintf(stderr, "ERROR: create_command_pool() failed.\n");
		return VK_NULL_HANDLE;
	}
	return pool;
}

VkCommandBuffer record_command_clear(VkDevice dev, VkCommandPool pool, VkImage img) {
	VkCommandBufferAllocateInfo info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	VkCommandBuffer cmdbuf;
	vkAllocateCommandBuffers(dev, &info, &cmdbuf);

	VkCommandBufferBeginInfo infoBegin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
	};
	vkBeginCommandBuffer(cmdbuf, &infoBegin);

	VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	VkClearColorValue color = {{0.8984375f, 0.8984375f, 0.9765625f, 1.0f}};
	vkCmdClearColorImage(cmdbuf, img,
	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);

	vkEndCommandBuffer(cmdbuf);
	return cmdbuf;
}

int vulkan_main() {
	VkInstance instance = create_instance();
	if (instance == VK_NULL_HANDLE)
		return EXIT_FAILURE;

	VkPhysicalDevice physical_device = get_physical_device(instance);
	if (physical_device == VK_NULL_HANDLE)
		return EXIT_FAILURE;

	VkSurfaceKHR surface = create_surface(instance, physical_device);
	if (surface == VK_NULL_HANDLE)
		return EXIT_FAILURE;

	VkDevice device = create_device(instance, physical_device);
	if (device == VK_NULL_HANDLE)
		return EXIT_FAILURE;

	VkSwapchainKHR swapchain = create_swapchain(physical_device, device, surface);
	if (swapchain == VK_NULL_HANDLE)
		return EXIT_FAILURE;

	VkQueue queue;
	vkGetDeviceQueue(device, 0, 0, &queue);

	uint32_t n = 1;
	VkImage image;
	vkGetSwapchainImagesKHR(device, swapchain, &n, &image);

	VkCommandPool command_pool = create_command_pool(device);
	if (command_pool == VK_NULL_HANDLE)
		return EXIT_FAILURE;

	VkCommandBuffer cmd_clear = record_command_clear(device, command_pool, image);

	uint32_t index;
	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &index);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd_clear
	};
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.swapchainCount = 1,
		.pSwapchains = &swapchain,
		.pImageIndices = &index
	};
	vkQueuePresentKHR(queue, &presentInfo);

	sleep(1);

	vkFreeCommandBuffers(device, command_pool, 1, &cmd_clear);

	vkDestroyCommandPool(device, command_pool, NULL);
	vkDestroySwapchainKHR(device, swapchain, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyDevice(device, NULL);
	vkDestroyInstance(instance, NULL);

	return EXIT_SUCCESS;
}
