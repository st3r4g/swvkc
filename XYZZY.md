# Running xyzzy in a minimal environment

Runs without `CONFIG_VT`.
Runs without `CONFIG_TTY` too, but terminal emulators won't work...

```
s6-mount -t proc proc /proc # Needed by... a lot of stuff
s6-mount -t sysfs sys /sys # Needed by libdrm (vulkan-intel)
mkdir -p -m0755 /dev/pts # Needed by terminal emulators
mount -o mode=0620,gid=5,nosuid,noexec -n -t devpts devpts /dev/pts
```

Needed by libinput (udev context):
```
udevd --daemon
udevadm trigger --action=add --type=subsystems
udevadm trigger --action=add --type=devices
udevadm settle
```
