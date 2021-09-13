project('libswvkc-wl', 'c', version: 'pre-alpha',
        default_options: ['c_std=c11', 'warning_level=3', 'werror=true'])
add_project_arguments('-pedantic-errors', '-Wno-unused-parameter',
                      '-Wno-unused-function', language: 'c')
add_project_link_arguments('-lm', language: 'c')

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG',language: 'c')
endif

c = 'core/'
u = 'util/'
x = 'extensions/'

src =  [c+'compositor.c', c+'data_device.c', c+'data_device_manager.c',
        c+'data_offer.c', c+'data_source.c', c+'keyboard.c', c+'output.c',
        c+'wl_pointer.c', c+'region.c', c+'seat.c', c+'wl_subcompositor.c',
        c+'subsurface.c', c+'wl_surface.c']
src += [u+'algebra.c', u+'log.c', u+'util.c']
src += [x+'xdg_shell/xdg_wm_base.c',
        x+'xdg_shell/xdg_positioner.c', x+'xdg_shell/xdg_surface.c',
        x+'xdg_shell/xdg_toplevel.c', x+'xdg_shell/xdg_popup.c',
        x+'linux-dmabuf-unstable-v1/zwp_linux_dmabuf_v1.c',
        x+'linux-dmabuf-unstable-v1/zwp_linux_buffer_params_v1.c',
        x+'linux-dmabuf-unstable-v1/wl_buffer_dmabuf.c',
        x+'linux-explicit-synchronization-v1/zwp_linux_explicit_synchronization_v1.c',
        x+'linux-explicit-synchronization-v1/zwp_linux_surface_synchronization_v1.c',
        x+'fullscreen-shell-unstable-v1/zwp_fullscreen_shell_v1.c',
        x+'server-decoration/org_kde_kwin_server_decoration_manager.c',
        x+'server-decoration/org_kde_kwin_server_decoration.c']
src += ['globals.c']

wayland_protocols_dep = dependency('wayland-protocols', native: true)
protocols_path = wayland_protocols_dep.get_pkgconfig_variable('pkgdatadir')

wayland_scanner_dep = dependency('wayland-scanner', native: true)
wayland_scanner = wayland_scanner_dep.get_pkgconfig_variable('wayland_scanner')

protocols = [
	'stable/xdg-shell/xdg-shell.xml',
	'unstable/fullscreen-shell/fullscreen-shell-unstable-v1.xml',
	'unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml',
	'unstable/linux-explicit-synchronization/linux-explicit-synchronization-unstable-v1.xml'
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

gen += custom_target(
	'server_decoration_xml_server_protocol',
	output: '@BASENAME@-server-protocol.h',
	input: 'server-decoration.xml',
	command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@']
)
gen += custom_target(
	'server_decoration_xml_code',
	output: '@BASENAME@-code.c',
	input: 'server-decoration.xml',
	command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@']
)

dep =  [wayland_protocols_dep]
dep += [wayland_scanner_dep]
dep += dependency('wayland-server')

inc = include_directories('include')

libswvkc_wl = static_library('swvkc-wl', [src, gen], dependencies: dep,
                             include_directories: inc)
libswvkc_wl_dep = declare_dependency(include_directories: inc,
                                     link_with: libswvkc_wl, sources: gen)
