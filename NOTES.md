## Issues with clients
* `vkcube` and `vkcubepp` have to be compiled with DEMOS_WSI_SELECTION=WAYLAND,
  but even then they won't work because they require the deprecated `wl_shell`
  protocol
  `Note! This protocol is deprecated and not intended for production use. For
  desktop-style user interfaces, use xdg_shell. ` from
  https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_shell

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

## Explicit syncronization
From https://www.collabora.com/news-and-blog/blog/2017/01/26/mainline-explicit-fencing-part-3/
> On the other hand for the fences created by the kernel that are sent back to
> userspace the OUT_FENCE_PTR property is used. It is a DRM CRTC property
> because we only create one dma_fence per CRTC as all the buffers on it will be
> scanned out at the same time. The kernel sends this fence back to userspace by
> writing the fd number to the pointer provided in the OUT_FENCE_PTR property.
> Note that, unlike from what Android did, when the fence signals it means the
> previous buffer – the buffer removed from the screen – is free for reuse. On
> Android when the signal was raised it meant the current buffer was freed.
> However, the Android folks have patched SurfaceFlinger already to support the
> Mainline semantics when using Explicit Fencing!

## TODO
* Code cleanup
* Investigate correct memory management and synchronization
* Direct scanout

## ISSUES/NOTES
* Many clients (`kitty`, `alacritty`, ...) still depend on the presence of the
  `wl_drm` 'legacy' extension to use dmabufs.
* Is VK_EXT_external_memory_host useful for us? The shm buffer from Alacritty
  could be imported in this way (only one time though), but not the ones from
  weston-terminal or weston-simple-shm.
* One gpu didn't support linear buffers for scanout, even though GBM allows
  their creation (error on drmModeAddFB)
