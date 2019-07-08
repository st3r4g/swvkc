#define _POSIX_C_SOURCE 200809L
#include <stdio.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <legacy_wl_drm.h>

static PFNEGLBINDWAYLANDDISPLAYWL eglBindWaylandDisplayWL = 0;

void legacy_wl_drm_setup(struct wl_display *D, struct gbm_device *gbm) {
	EGLDisplay display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_MESA, gbm,
	NULL);
	if (display == EGL_NO_DISPLAY) {
		fprintf(stderr, "eglGetPlatformDisplay failed\n");
	}

	EGLint major, minor;
	if (eglInitialize(display, &major, &minor) == EGL_FALSE) {
		fprintf(stderr, "eglInitialize failed\n");
	}
	printf("EGL %i.%i initialized\n", major, minor);

	eglBindWaylandDisplayWL = (PFNEGLBINDWAYLANDDISPLAYWL)
	eglGetProcAddress("eglBindWaylandDisplayWL");

	if (eglBindWaylandDisplayWL(display, D) == EGL_FALSE) {
		fprintf(stderr, "eglBindWaylandDisplayWL failed\n");
	}
}
