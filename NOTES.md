## Vulkan extensions that we need
* VK_KHR_external_memory_fd
* VK_EXT_external_memory_dma_buf
* VK_EXT_image_drm_format_modifier [not yet implemented by Mesa]

## Steps to import a dma_buf into the Vulkan compositor
1. Create a buffer, which is initially a virtual allocation with no backing
memory
`VkImage = vkCreateImage()`
2. Allocate device memory from the dma_buf fd
`VkDeviceMemory = vkAllocateMemory(fd)`
3. Associate the device memory with the buffer
`vkBindImageMemory(VkImage, VkDeviceMemory)`

## Steps to import a shm buffer into the Vulkan compositor
1. Get the data pointer from the shm buffer
`data = wl_shm_buffer_get_data(shm_buffer)`
2. Copy the data into a staging buffer (can this be avoided?)
`memcpy(dest, data)`

## Steps to export a Vulkan rendered buffer (the screen) to DRM
1. Create a bo with GBM
`bo = gbm_bo_create()`
2. Get the bo fd, which should be a dma_buf fd
`fd = gbm_bo_get_fd(bo)`
3. Create a buffer, which is initially a virtual allocation with no backing
memory
`VkImage = vkCreateImage()`
4. Allocate device memory from the dma_buf fd
`VkDeviceMemory = vkAllocateMemory(fd)`
5. Associate the device memory with the buffer
`vkBindImageMemory(VkImage, VkDeviceMemory)`
6. Render to the buffer
`vkCmdClearColorImage`
7. Retrieve the bo handle to feed it to DRM
`handle = gbm_bo_get_handle(bo).u32`

## TODO
* ~~Implement `create` request in the dmabuf Wayland interface~~
* ~~Visualize `weston-simple-dmabuf-drm` buffers~~
* Import shm buffers

## ISSUES
* Is VK_EXT_external_memory_host useful for us? The shm buffer from Alacritty
  could be imported in this way (only one time though), but not the ones from
  weston-terminal or weston-simple-shm.
