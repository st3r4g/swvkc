## Vulkan extensions that we need
* VK_EXT_external_memory_host
* VK_KHR_external_memory_fd
* VK_EXT_external_memory_dma_buf
* VK_EXT_image_drm_format_modifier [not implemented by Mesa yet I believe]

## Steps to import a dma_buf into the Vulkan compositor
1. Create a buffer, which is initially a virtual allocation with no backing
memory
`VkBuffer = vkCreateBuffer()`
2. Allocate device memory from the dma_buf fd
`VkDeviceMemory = vkAllocateMemory(fd)`
3. Associate the device memory with the buffer
`vkBindBufferMemory(VkBuffer, VkDeviceMemory)`

## Steps to import a shm buffer into the Vulkan compositor
1. Same as above
2. Allocate device memory from the host pointer
`VkDeviceMemory = vkAllocateMemory(data)`
3. Same as above

## Steps to export a Vulkan rendered buffer (the screen) to DRM
1. Create a bo with GBM
`bo = gbm_bo_create()`
2. Get the bo fd, which should be a dma_buf fd
`fd = gbm_bo_get_fd(bo)`
3. Create a buffer, which is initially a virtual allocation with no backing
memory
`VkBuffer = vkCreateBuffer()`
4. Allocate device memory from the dma_buf fd
`VkDeviceMemory = vkAllocateMemory(fd)`
5. Associate the device memory with the buffer
`vkBindBufferMemory(VkBuffer, VkDeviceMemory)`
6. Render to the buffer
`vkCmdFillBuffer`
7. Retrieve the bo handle to feed it to DRM
`handle = gbm_bo_get_handle(bo).u32`

## TODO
* ~~Implement `create` request in the dmabuf Wayland interface~~
* ~~Visualize `weston-simple-dmabuf-drm` buffers~~
