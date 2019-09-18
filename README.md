# swvkc (temporary name)
`swvkc` is a Wayland compositor which sacrifices features for performance and
correctness.\
`swvkc` prioritizes direct scanout of client buffers when possible. When
compositing needs to be done, it renders to the screen with simple copy commands
from the Vulkan API.\
An explicit list of the goals of the project:
* Do the minimal work necessary to display client buffers, do not introduce
  screen tearing/stuttering or input lag.
* Strictly adhere to the Wayland protocol specification in order to minimize
  undefined behaviour and assure stability.
* Write simple, readable and standardized code.

Things like simple window management features will of course eventually be
implemented but I don't want to rush to avoid bugs.

## Current status
`swvkc` is in Alpha stage. The whole idea of a DRM+Vulkan compositor was quite
experimental, so I quickly sketched out things to see if they worked without
caring too much for correctness. Now I've reached a point in which the program
is actually usable (with some caveats), so experimentation and consolidation
will proceed in parallel.\
Some considerations:
* The mechanism to import both shm and dma buffers into Vulkan has been quickly
  sketched, but it needs to be made more robust (some assumptions about the
  buffer formats should be removed too).
* I tried for some time to implement tear-free single-buffered rendering (by
  restricting rendering to VBlank Intervals) but failed to achieve it
  consistently. Using double-buffering for now.
* The Wayland protocol has been implemented partially so only some clients work
  at the moment.

## Current features (or lack thereof)
* Only one surface is focused and displayed at a time
* One single client can be run at startup (I suggest a terminal)
* Focus can be changed (see Usage notes)
* Keyboard only (no pointer) [coming soon...]
* No popups [coming soon...]
* No clipboard [coming soon...]

## Constraints
* The hardware is required to support the modern DRM atomic commit interface
* No XWayland support

## Dependencies
`swvkc` tries to have minimal dependencies. Here is the full list:
* Wayland
  * `wayland-server`, the core server API
  * `wayland-protocols` to generate protocol headers and code
* DRM
  * `libdrm` for controlling the screen
  * `libdrm_intel` (optional) to use the intel buffer manager instead of GBM
* Vulkan
  * `vulkan-loader` to load the appropriate vendor-specific libraries
  * the vendor-specific Vulkan implementation (e.g.: `vulkan_intel` or `amdvlk`)
* Mesa
  * `gbm` for the GBM buffer manager but also required by `egl`
  * `egl` [legacy] I have to keep it to provide the `wl_drm` interface, on which
    Mesa relies to authenticate against DRM
* `xkbcommon` for keyboard standardization, it is practically required by the
  Wayland spec
* `libudev` for GPU discovery and monitor hotplugging (not yet)

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

## Usage notes
* The presence of the environment variable `XDG_RUNTIME_DIR` is required, and it
  must point to an appropriate folder (this is automatically set up by `systemd`
  or `elogind`)
* Provide the required permissions (e.g. by adding the user to group video and
  input)
* Start the program from the Linux console (not inside another X11/Wayland
  compositor)
* Pass it the client (and its arguments) you want to run
* Press F1 to exit [will change soon]
* To change focus: hold the LEFTMETA (Win) key and start to type the client name
  until an unambiguous match is found
* Keyboard settings (such as layout) are set through XKB_DEFAULT\* environmental
  variables.

## An issue
Importing memory created by DRM/GBM into Vulkan images is undefined behaviour
without the `VK_EXT_image_drm_format_modifier` extension: even when the image
is created with linear layout and without modifiers, there is no way to
specify the correct row pitch (please correct me if I'm wrong):\
From Issues in https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap44.html#VK_EXT_external_memory_dma_buf
> How does the application, when creating a VkImage that it intends to bind to
> dma_buf VkDeviceMemory containing an externally produced image, specify the
> memory layout (such as row pitch and DRM format modifier) of the VkImage? In
> other words, how does the application achieve behavior comparable to that
> provided by EGL_EXT_image_dma_buf_import and
> EGL_EXT_image_dma_buf_import_modifiers?\
> +\
> RESOLVED. Features comparable to those in EGL_EXT_image_dma_buf_import and
> EGL_EXT_image_dma_buf_import_modifiers will be provided by an extension layered
> atop this one.

The extension mentioned in the last line is precisely
`VK_EXT_image_drm_format_modifier`. If you are unable to display clients like
`weston-simple-shm`, `weston-terminal` and `firefox`, the absence of this
extension is likely to be the problem. If you are using an Intel GPU and don't
want to wait for Mesa to implement the extension, consider following the
paragraph below.

### Patching and compiling Mesa
A working implementation for the Intel Vulkan driver (anv) exists but has not
been merged yet in Mesa's release branch: https://gitlab.freedesktop.org/mesa/mesa/merge_requests/515 \
I created a patch containing the commits from the link above merged on top of
the latest Mesa. I'll try to keep it updated as new versions of Mesa get
released until the code gets merged upstream.\
Here is the [PATCH](https://mega.nz/#!a1pznIZQ!bfJaxpuxOeWy4N8qQdZrUHFu5zwWtInFXvDbKWH2MCM)
(verified to build against versions 19.1.x)
