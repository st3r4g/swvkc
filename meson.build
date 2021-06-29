project('swvkc', 'c', version: 'pre-alpha',
        default_options: ['c_std=c11', 'warning_level=3', 'werror=true'])
add_project_arguments('-pedantic-errors', '-Wno-unused-parameter',
                      '-Wno-unused-function', language : 'c')

wl_proj = subproject('libswvkc-wl')
wl_dep = wl_proj.get_variable('libswvkc_wl_dep')

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
