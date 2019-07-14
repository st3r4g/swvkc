project('swvkc', 'c', default_options : ['c_std=c11', 'warning_level=3',
'werror=true'])
add_project_arguments('-pedantic-errors', '-Wno-unused-parameter',
'-Wno-unused-function', language : 'c')
add_project_link_arguments('-lm', language : 'c')

b = 'backend/'
c = 'core/'
u = 'util/'
x = 'extensions/'

src = [b+'input.c', b+'screen.c', b+'vulkan.c']
src += [c+'compositor.c', c+'data_device_manager.c', c+'keyboard.c',
	c+'output.c', c+'wl_pointer.c', c+'region.c', c+'seat.c',
	c+'wl_subcompositor.c', c+'wl_subsurface.c', c+'wl_surface.c']
src += [u+'algebra.c', u+'log.c']
src += [x+'xdg_shell/xdg_wm_base.c',
	x+'xdg_shell/xdg_positioner.c', x+'xdg_shell/xdg_surface.c',
	x+'xdg_shell/xdg_toplevel.c', x+'xdg_shell/xdg_popup.c',
	x+'linux-dmabuf-unstable-v1/zwp_linux_dmabuf_v1.c',
	x+'linux-dmabuf-unstable-v1/zwp_linux_buffer_params_v1.c',
	x+'linux-dmabuf-unstable-v1/wl_buffer_dmabuf.c']
src += ['legacy_wl_drm.c', 'main.c']

wayland_protocols = dependency('wayland-protocols')
path = wayland_protocols.get_pkgconfig_variable('pkgdatadir')

wayland_scanner_dep = dependency('wayland-scanner', required: false, native: true)
if wayland_scanner_dep.found()
	wayland_scanner = find_program(
		wayland_scanner_dep.get_pkgconfig_variable('wayland_scanner'),
		native: true,
	)
else
	wayland_scanner = find_program('wayland-scanner', native: true)
endif

xdg_shell_server_protocol = custom_target(
	'xdg-shell-server-protocol',
	output: 'xdg-shell-server-protocol.h',
	input: path/'stable/xdg-shell/xdg-shell.xml',
	command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

xdg_shell_code = custom_target(
	'xdg-shell-code',
	output: 'xdg-shell-code.c',
	input: path/'stable/xdg-shell/xdg-shell.xml',
	command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

fullscreen_shell_unstable_v1_server_protocol = custom_target(
	'fullscreen-shell-unstable-v1-server-protocol',
	output: 'fullscreen-shell-unstable-v1-server-protocol.h',
	input: path/'unstable/fullscreen-shell/fullscreen-shell-unstable-v1.xml',
	command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

fullscreen_shell_unstable_v1_code = custom_target(
	'fullscreen-shell-unstable-v1-code',
	output: 'fullscreen-shell-unstable-v1-code.c',
	input: path/'unstable/fullscreen-shell/fullscreen-shell-unstable-v1.xml',
	command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

linux_dmabuf_unstable_v1_server_protocol = custom_target(
	'linux-dmabuf-unstable-v1-server-protocol',
	output: 'linux-dmabuf-unstable-v1-server-protocol.h',
	input: path/'unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml',
	command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

linux_dmabuf_unstable_v1_code = custom_target(
	'linux-dmabuf-unstable-v1-code',
	output: 'linux-dmabuf-unstable-v1-code.c',
	input: path/'unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml',
	command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

src += xdg_shell_server_protocol
src += xdg_shell_code
src += fullscreen_shell_unstable_v1_server_protocol
src += fullscreen_shell_unstable_v1_code
src += linux_dmabuf_unstable_v1_server_protocol
src += linux_dmabuf_unstable_v1_code

dep = [wayland_protocols]
dep += dependency('egl') # required for legacy wl_drm
dep += dependency('gbm')
dep += dependency('libdrm')
dep += dependency('libudev')
dep += dependency('vulkan')
dep += dependency('wayland-server')
dep += dependency('xkbcommon')

inc = include_directories('include')

executable('swvkc', src, dependencies : dep, include_directories : inc)
