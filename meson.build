project('swvkc', 'c', default_options : ['c_std=c11', 'warning_level=3',
'werror=true'])
add_project_arguments('-pedantic-errors', '-Wno-unused-parameter',
'-Wno-unused-function', language : 'c')
add_project_link_arguments('-lm', language : 'c')

b = 'backend/'
c = 'core/'
u = 'util/'
x = 'extensions/'

src = [b+'bufmgr_intel.c', b+'input.c', b+'screen.c', b+'vulkan.c']
src += [c+'compositor.c', c+'data_device_manager.c', c+'keyboard.c',
	c+'output.c', c+'wl_pointer.c', c+'region.c', c+'seat.c',
	c+'wl_subcompositor.c', c+'subsurface.c', c+'wl_surface.c']
src += [u+'algebra.c', u+'log.c', u+'util.c', u+'my_drm_handle_event.c']
src += [x+'xdg_shell/xdg_wm_base.c',
	x+'xdg_shell/xdg_positioner.c', x+'xdg_shell/xdg_surface.c',
	x+'xdg_shell/xdg_toplevel.c', x+'xdg_shell/xdg_popup.c',
	x+'linux-dmabuf-unstable-v1/zwp_linux_dmabuf_v1.c',
	x+'linux-dmabuf-unstable-v1/zwp_linux_buffer_params_v1.c',
	x+'linux-dmabuf-unstable-v1/wl_buffer_dmabuf.c',
	x+'fullscreen-shell-unstable-v1/zwp_fullscreen_shell_v1.c']
src += ['legacy_wl_drm.c', 'main.c']

wayland_protocols = dependency('wayland-protocols')
protocols_path = wayland_protocols.get_pkgconfig_variable('pkgdatadir')

wayland_scanner_dep = dependency('wayland-scanner', required: false, native: true)
if wayland_scanner_dep.found()
	wayland_scanner = find_program(
		wayland_scanner_dep.get_pkgconfig_variable('wayland_scanner'),
		native: true,
	)
else
	wayland_scanner = find_program('wayland-scanner', native: true)
endif

protocols = [
	'stable/xdg-shell/xdg-shell.xml',
	'unstable/fullscreen-shell/fullscreen-shell-unstable-v1.xml',
	'unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml'
]

gen = []

foreach p : protocols
	gen += custom_target(
		p.underscorify()+'_server_protocol',
		output: '@BASENAME@-server-protocol.h',
		input: protocols_path/p,
		command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@']
	)
	gen += custom_target(
		p.underscorify()+'_code',
		output: '@BASENAME@-code.c',
		input: protocols_path/p,
		command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@']
	)
endforeach

dep = [wayland_protocols]
dep += dependency('wayland-server')
dep += dependency('vulkan')
dep += dependency('libdrm')
dep += dependency('xkbcommon')
dep += dependency('libudev')
dep += dependency('egl') # required for legacy wl_drm
dep += dependency('gbm') # required for legacy wl_drm and for bufmgr_gbm
dep += dependency('libdrm_intel') # required for bufmgr_intel

inc = include_directories('include')

executable('swvkc', [src, gen], dependencies : dep, include_directories : inc, install : true)
