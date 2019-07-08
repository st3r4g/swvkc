# swvkc (temporary name)
swvkc is an attempt to write a simple Wayland compositor. Rendering is done via
Vulkan and input is acquired directly from the kernel. It is still very WIP and
should not be used until the implementation of the most important Wayland
interfaces is completed.

## Constraints
* The hardware is required to support the modern DRM atomic commit interface
* No XWayland support

## Usage notes
* Only start the program in a virtual terminal (not inside another X11/Wayland
compositor)
* Press F1 to exit
* Press F2 to cycle between clients
* Keyboard settings are set through XKB_DEFAULT\* environmental variables.
* Pass the client you want to run as an argument (usually a terminal emulator)

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
