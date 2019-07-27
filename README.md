# swvkc (temporary name)
`swvkc` is a WIP experimental Wayland compositor which aims to be minimalistic
but efficient. Compositing is done by importing the clients video buffers into
Vulkan and copying them onto the screen buffer.

# Current status
* The mechanism to import both shm and dma buffers into Vulkan has been quickly
  sketched, now I'm trying to make it more robust (some assumptions about the
  buffer formats should be removed too).
* The Wayland protocol has been implemented partially so only simple specific
  clients work at the moment. I'm currently interested in the buffer sharing
  mechanism and all the interactions between DRM, GBM and Vulkan.

## Constraints
* The hardware is required to support the modern DRM atomic commit interface
* No XWayland support

## Usage notes
* Add the user to group input to allow swvkc to grab the keyboard (group video
  membership is not required)
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
