#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <vulkan/vulkan.h>
#include <util/log.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
VkDebugUtilsMessageTypeFlagsEXT messageType, const
VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);
	return VK_FALSE;
}


VkInstance create_instance() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo2 = {0};
	createInfo2.sType =
	VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo2.messageSeverity =
	VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo2.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo2.pfnUserCallback = debugCallback;


	const char *extensions[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_DISPLAY_EXTENSION_NAME,
		VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
	};
	const char *layers[] = {"VK_LAYER_LUNARG_standard_validation"};
	VkInstanceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = &createInfo2,
		.enabledLayerCount = sizeof(layers)/sizeof(char*),
		.ppEnabledLayerNames = layers,
		.enabledExtensionCount = sizeof(extensions)/sizeof(char*),
		.ppEnabledExtensionNames = extensions
	};
	VkInstance inst;
	if (vkCreateInstance(&info, NULL, &inst)) {
		fprintf(stderr, "ERROR: create_instance() failed.\n");
		return VK_NULL_HANDLE;
	}

/*	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger =
	(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(inst,
	"vkCreateDebugUtilsMessengerEXT");
	VkDebugUtilsMessengerEXT debugMessenger;
	vkCreateDebugUtilsMessenger(inst, &createInfo2, 0, &debugMessenger);*/

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
		VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
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

VkImage create_image(int32_t width, int32_t height, VkDevice dev) {
	VkExternalMemoryImageCreateInfo info_next = {
		.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
		.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
	};
	// I must convert from DRM to Vk formats
	// I must choose the correct tiling
	// I must choose the correct src/dst
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = &info_next,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_B8G8R8A8_UNORM, // TODO
		.extent = {width, height, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_LINEAR, // TODO
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT, // or SRC
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
//		.queueFamilyIndexCount = 0, ignored
//		.pQueueFamilyIndices = 0, ignored
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	VkImage img;
	if (vkCreateImage(dev, &info, NULL, &img)) {
		fprintf(stderr, "ERROR: create_image() failed.\n");
		return VK_NULL_HANDLE;
	}
	return img;
}

// Pass stride * height as size
VkDeviceMemory import_memory(int fd, int size, VkDevice dev) {
	VkImportMemoryFdInfoKHR info_next = {
		.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
		.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
// or maybe VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
		.fd = fd,
	};
	VkMemoryAllocateInfo info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &info_next,
		.allocationSize = size
	};
	VkDeviceMemory mem;
	VkResult res = vkAllocateMemory(dev, &info, NULL, &mem);
	if (res) {
		fprintf(stderr, "ERROR: import_memory(%d, %d) failed. %d\n", fd,
		size, res == VK_ERROR_INVALID_EXTERNAL_HANDLE);

		return VK_NULL_HANDLE;
	}
	return mem;
}



VkResult bind_image_memory(VkDevice dev, VkImage img, VkDeviceMemory mem) {
	return vkBindImageMemory(dev, img, mem, 0);
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

int32_t data[2] = {0xFF00FFFF, 0xFF0000FF};

VkCommandBuffer record_command_clear2(VkDevice dev, VkCommandPool pool, VkBuffer buf) {
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

	vkCmdFillBuffer(cmdbuf, buf, 0, VK_WHOLE_SIZE, data[0]);

	vkEndCommandBuffer(cmdbuf);
	return cmdbuf;
}

VkCommandBuffer record_command_clear3(VkDevice dev, VkCommandPool pool, VkBuffer buf) {
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

	vkCmdFillBuffer(cmdbuf, buf, 0, VK_WHOLE_SIZE, data[1]);

	vkEndCommandBuffer(cmdbuf);
	return cmdbuf;
}

VkCommandBuffer record_command_copy(VkDevice dev, VkCommandPool pool, VkImage
img, VkImage img2) {
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

	VkImageSubresourceLayers imageSubresource = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.mipLevel = 0,
		.baseArrayLayer = 0,
		.layerCount = 1
	};

	VkImageCopy region = {
		.srcSubresource = imageSubresource,
		.srcOffset = {0,0,0},
		.dstSubresource = imageSubresource,
		.dstOffset = {200,100,0},
		.extent = {1366,768,0},
	};
	vkCmdCopyImage(cmdbuf, img,
	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img2,
	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	vkEndCommandBuffer(cmdbuf);
	return cmdbuf;
}

VkCommandBuffer record_command_copy2(VkDevice dev, VkCommandPool pool, VkBuffer
buf, VkBuffer buf2) {
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

	VkBufferCopy region = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = 262144,
	};
	vkCmdCopyBuffer(cmdbuf, buf, buf2, 1, &region);

	vkEndCommandBuffer(cmdbuf);
	return cmdbuf;
}

VkDevice device;
VkQueue queue;
VkCommandPool command_pool;
VkCommandBuffer commands[2] = {0,0};
VkImage screen_image;


int vulkan_init(int fd) {
	VkInstance instance = create_instance();
	if (instance == VK_NULL_HANDLE)
		return EXIT_FAILURE;
	printf("Instance created\n");

	VkPhysicalDevice physical_device = get_physical_device(instance);
	if (physical_device == VK_NULL_HANDLE)
		return EXIT_FAILURE;
	printf("physical device created\n");

/*	VkSurfaceKHR surface = create_surface(instance, physical_device);
	if (surface == VK_NULL_HANDLE)
		return EXIT_FAILURE;*/

	device = create_device(instance, physical_device);
	if (device == VK_NULL_HANDLE)
		return EXIT_FAILURE;

/*	VkSwapchainKHR swapchain = create_swapchain(physical_device, device, surface);
	if (swapchain == VK_NULL_HANDLE)
		return EXIT_FAILURE;*/

	vkGetDeviceQueue(device, 0, 0, &queue);

/*	uint32_t n = 1;
	VkImage image;
	vkGetSwapchainImagesKHR(device, swapchain, &n, &image);*/

	command_pool = create_command_pool(device);
	if (command_pool == VK_NULL_HANDLE)
		return EXIT_FAILURE;

//	commands[0] = record_command_clear(device, command_pool, image);

	/*
	 * Trying to post the dmabuf
	 */

	screen_image = create_image(1920, 1080, device);
	VkDeviceMemory screen_memory = import_memory(fd, 8294400, device);
	bind_image_memory(device, screen_image, screen_memory);
	commands[0] = record_command_clear(device, command_pool, screen_image);
	return EXIT_SUCCESS;
}
int vulkan_main(int i, int fd, int width, int height, int stride) {
	fd = dup(fd); // otherwise I can't import the same fd again
	// END

/*	uint32_t index;
	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &index);*/
VkImage image;
VkDeviceMemory memory;
	if (i == 1) {
	image = create_image(width, height, device);
	memory = import_memory(fd, stride*height, device);
	bind_image_memory(device, image, memory);

		commands[1] = record_command_copy(device, command_pool, image,
		screen_image);
	}

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = commands+i
	};
	VkFence fence;
	VkFenceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
		};
	vkCreateFence(device, &info, NULL, &fence);
	errlog("COPY START");
	vkQueueSubmit(queue, 1, &submitInfo, fence);
	vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000);
	errlog("COPY DONE");
	vkResetFences(device, 1, &fence);

	if (i == 1) {
		vkDestroyImage(device, image, NULL);
		vkFreeMemory(device, memory, NULL);
		}

	close(fd);


/*	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.swapchainCount = 1,
		.pSwapchains = &swapchain,
		.pImageIndices = &index
	};
	vkQueuePresentKHR(queue, &presentInfo);*/

//	vkFreeCommandBuffers(device, command_pool, 1, commands);

//	vkDestroyCommandPool(device, command_pool, NULL);
/*	vkDestroySwapchainKHR(device, swapchain, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);*/
//	vkDestroyDevice(device, NULL);
//	vkDestroyInstance(instance, NULL);

	return EXIT_SUCCESS;
}
