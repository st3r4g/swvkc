## Vulkan extensions that we need
* VK_KHR_external_memory_fd
* VK_EXT_external_memory_dma_buf
* VK_EXT_image_drm_format_modifier [optional]

## Steps to import a dma_buf into the Vulkan compositor
1. Create a buffer, which is initially a virtual allocation with no backing
memory
`VkBuffer = vkCreateBuffer()`
2. Allocate device memory from the dma_buf fd
`VkDeviceMemory = vkAllocateMemory(fd)`
3. Associate the device memory with the buffer
`vkBindBufferMemory(VkBuffer, VkDeviceMemory)`

## TODO
* ~~Implement `create` request in the dmabuf Wayland interface~~
* Visualize `weston-simple-dmabuf-drm` buffers
