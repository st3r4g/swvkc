# swvkc (temporary name)
`swvkc` is a WIP experimental Wayland compositor which aims to be minimalistic
but efficient. Compositing is done by importing the clients video buffers into
Vulkan and copying them onto the screen buffer.

# Current status
* Importing memory between Vulkan and DRM/GBM is a bit hit-and-miss without the
  `VK_EXT_image_drm_format_modifier` extension. A working implementation for the
  intel vulkan driver exists but has not been merged yet in mesa.
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
