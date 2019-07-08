# swvkc (temporary name)
`swvkc` is an experimental Wayland compositor which aims to be minimalistic but
efficient. Compositing is done by importing the clients video buffers into
Vulkan and copying them onto the screen buffer.

# Current status
WARNING! The software is extremely early stage.
* `weston-simple-dmabuf-drm` buffer successfully presented to the screen (which
  is still a VkSwapchain buffer, one of the next steps would be to see if it's
  possible to use a 'simple' buffer instead, manually feeding it to GBM/DRM).

# General description

## Constraints
* The hardware is required to support the modern DRM atomic commit interface
* No XWayland support

## Usage notes
* Only start the program in a virtual terminal (not inside another X11/Wayland
compositor)
* Pass the client you want to run as an argument
* Press F1 to exit
* ~~Press F2 to cycle between clients~~ (not yet)
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
