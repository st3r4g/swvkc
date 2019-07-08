project('swvkc', 'c', default_options : ['c_std=c11', 'warning_level=3',
'werror=true'])
add_project_arguments('-pedantic-errors', '-Wno-unused-parameter',
'-Wno-unused-function', language : 'c')
add_project_link_arguments('-lm', language : 'c')

b = 'backend/'
c = 'core/'
p = 'protocols/'
u = 'util/'
x = 'extensions/'

src = [b+'input.c', b+'screen.c']
src += [c+'compositor.c', c+'data_device_manager.c', c+'keyboard.c',
	c+'output.c', c+'wl_pointer.c', c+'region.c', c+'seat.c',
	c+'wl_subcompositor.c', c+'wl_subsurface.c', c+'wl_surface.c']
src += [p+'xdg-shell-unstable-v6-code.c', p+'xdg-shell-code.c',
	p+'linux-dmabuf-unstable-v1-code.c', p+'fullscreen-shell-unstable-v1.c']
src += [u+'algebra.c', u+'log.c']
src += [x+'xdg_shell/xdg_wm_base.c',
	x+'xdg_shell/xdg_positioner.c', x+'xdg_shell/xdg_surface.c',
	x+'xdg_shell/xdg_toplevel.c', x+'xdg_shell/xdg_popup.c',
	x+'linux-dmabuf-unstable-v1/zwp_linux_dmabuf_v1.c',
	x+'linux-dmabuf-unstable-v1/zwp_linux_buffer_params_v1.c',
	x+'linux-dmabuf-unstable-v1/wl_buffer_dmabuf.c']
src += ['legacy_wl_drm.c', 'my_vulkan.c', 'main.c']

inc = include_directories('include')

dep = [dependency('egl')] # required for legacy wl_drm
dep += dependency('gbm')
dep += dependency('libdrm')
dep += dependency('libudev')
dep += dependency('vulkan')
dep += dependency('wayland-server')
dep += dependency('xkbcommon')

executable('swvkc', src, dependencies : dep, include_directories : inc)
