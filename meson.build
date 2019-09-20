project('swvkc', 'c', version: 'pre-alpha',
        default_options: ['c_std=c11', 'warning_level=3', 'werror=true'])
add_project_arguments('-pedantic-errors', '-Wno-unused-parameter',
                      '-Wno-unused-function', language : 'c')

wl_proj = subproject('libswvkc-wl')
wl_dep = wl_proj.get_variable('libswvkc_wl_dep')

b = 'backend/'
u = 'util/'

bufmgr = get_option('bufmgr')

src =  [b+'bufmgr_'+bufmgr+'.c', b+'dev.c', b+'input.c', b+'screen.c',
        b+'vulkan.c']
src += [u+'my_drm_handle_event.c']
src += ['legacy_wl_drm.c', 'main.c']

dep = [wl_dep]
dep += dependency('egl') # required for legacy wl_drm
dep += dependency('gbm') # required by egl
dep += dependency('libdrm')
dep += dependency('libdrm_intel', required: bufmgr == 'intel')
dep += dependency('libudev')
dep += dependency('vulkan')
dep += dependency('xkbcommon')

inc = include_directories('include')

executable('swvkc', src, dependencies: dep, include_directories: inc,
           install: true)

summary = [
	'',
	'----------------',
	'swvkc @0@'.format(meson.project_version()),
	'',
	' bufmgr (buffer manager): @0@'.format(bufmgr),
	'----------------',
	''
]
message('\n'.join(summary))
