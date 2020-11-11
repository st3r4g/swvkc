#define _POSIX_C_SOURCE 200809L
#include <stdio.h>

#include <gbm.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglmesaext.h>

#include <legacy_wl_drm.h>

static PFNEGLBINDWAYLANDDISPLAYWL eglBindWaylandDisplayWL = 0;

void legacy_wl_drm_setup(struct wl_display *D, int drm_fd) {
	struct gbm_device *gbm = gbm_create_device(drm_fd);
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
	if (!eglBindWaylandDisplayWL)
		fprintf(stderr, "eglBindWaylandDisplayWL is NULL\n");

	if (eglBindWaylandDisplayWL(display, D) == EGL_FALSE) {
		fprintf(stderr, "eglBindWaylandDisplayWL failed\n");
	}
}
