project('swvkc', 'c', version: 'pre-alpha',
        default_options: ['c_std=c11', 'warning_level=3', 'werror=true'])
add_project_arguments('-pedantic-errors', '-Wno-unused-parameter',
                      '-Wno-unused-function', language : 'c')

wl_proj = subproject('libswvkc-wl')
wl_dep = wl_proj.get_variable('libswvkc_wl_dep')

if get_option('xyzzy')
	xyzzy_src = files(['input.c', 'gbm.c', 'loop.c', 'modeset.c', 'wayland.c', 'xkb.c', 'legacy_wl_drm.c'])

	cc = meson.get_compiler('c')
	skalibs = cc.find_library('skarnet')

	xyzzy_dep = [skalibs]
	xyzzy_dep += dependency('egl')
	xyzzy_dep += dependency('gbm')
	xyzzy_dep += dependency('libdrm')
	xyzzy_dep += dependency('libinput')
	xyzzy_dep += dependency('libudev')
	xyzzy_dep += dependency('xkbcommon')
	xyzzy_dep += wl_proj.get_variable('libswvkc_wl_dep')

	xyzzy_inc = []

	executable('xyzzy', xyzzy_src, dependencies: xyzzy_dep, include_directories: xyzzy_inc, c_args: ['-Wno-error=format=', '-Wno-error=sign-compare', '-Wno-error=unused-variable'])
endif

src = []
subdir('src')

dep = [wl_dep]
dep += dependency('egl') # required for legacy wl_drm
dep += dependency('gbm') # required by egl
dep += dependency('libdrm')
dep += dependency('libdrm_intel', required: bufmgr == 'intel')
dep += dependency('libudev')
dep += dependency('vulkan', version : '>=1.2.182')
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
