# swvkc (temporary name)
`swvkc` is a WIP experimental Wayland compositor which aims to be minimalistic
but efficient. Compositing is done by importing the clients video buffers into
Vulkan and copying them onto the screen buffer.

# Current status
* Importing memory created by DRM/GBM into Vulkan images is undefined behaviour
  without the `VK_EXT_image_drm_format_modifier` extension: even when the image
  is created with linear layout and without modifiers, there is no way to
  specify the correct row pitch:
  From Issues in https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap44.html#VK_EXT_external_memory_dma_buf
> How does the application, when creating a VkImage that it intends to bind to
> dma_buf VkDeviceMemory containing an externally produced image, specify the
> memory layout (such as row pitch and DRM format modifier) of the VkImage? In
> other words, how does the application achieve behavior comparable to that
> provided by EGL_EXT_image_dma_buf_import and
> EGL_EXT_image_dma_buf_import_modifiers?
> +
> RESOLVED. Features comparable to those in EGL_EXT_image_dma_buf_import and
> EGL_EXT_image_dma_buf_import_modifiers will be provided by an extension layered
> atop this one.

  The extension mentioned in the last line is precisely
  `VK_EXT_image_drm_format_modifier`. A working implementation for the Intel
  Vulkan driver (anv) exists but has not been merged yet in Mesa:
  https://gitlab.freedesktop.org/mesa/mesa/merge_requests/515
  I created a patch containing the commits from the link above merged on top of
  the latest Mesa. I'll try to keep it updated as new versions of Mesa get
  released until the code gets merged upstream. The current patch was verified
  to build against Mesa versions 19.1.4 and 19.1.6:
  https://mega.nz/#!T0h0QYjS!VK6js7j3u3xYUULTTGtDGNnscYk2CF1ibm-ZTfsxgP0
* The mechanism to import both shm and dma buffers into Vulkan has been quickly
  sketched, but it needs to be made more robust (some assumptions about the
  buffer formats should be removed too). Addressing this once the issue above is
  solved.
* I tried for some time to implement tear-free single-buffered rendering (by
  restricting rendering to VBlank Intervals) but failed to achieve it
  consistently. Using double-buffering for now.
* The Wayland protocol has been implemented partially so only some clients work
  at the moment.

## Constraints
* The hardware is required to support the modern DRM atomic commit interface
* No XWayland support

## Usage notes
* Add the user to group video and input
* Start the program in a virtual terminal (not inside another X11/Wayland
  compositor)
* Pass the client you want to run as an argument
* Press F1 to exit
* To change focus: hold the LEFTMETA (Win) key and start to type the client name
* Keyboard settings are set through XKB_DEFAULT\* environmental variables.

## Building
After installing `meson`, run the command:
```
$ meson build
```
and install any missing dependency that is reported. Then run:
```
$ ninja -C build
```
Upon successful compilation the binary is located inside the `build` directory.
