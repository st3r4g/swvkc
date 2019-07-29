#include <intel_bufmgr.h>
#include <i915_drm.h>

drm_intel_bufmgr *bufmgr = 0;
drm_intel_bo *intel_bo = 0;

int bpp = 32;
uint32_t tiling = I915_TILING_NONE;

long unsigned int stride;

int b(int width, int height) {
	bufmgr = drm_intel_bufmgr_gem_init(fd, 32);
	if (!bufmgr) {
		fprintf(stderr, "drm_intel_bufmgr_gem_init failed\n");
		return -1;
	}

	intel_bo = drm_intel_bo_alloc_tiled(bufmgr, "main", width, height,
	 bpp/8, &tiling, &stride, 0);
	if (!intel_bo) {
		fprintf(stderr, "drm_intel_bo_alloc failed\n");
		return -1;
	}
	printf("success, stride: %lu\n", stride);
}

uint32_t get_handle() {
	return intel_bo->handle;
}

void y() {
	drm_intel_bo_unreference(intel_bo);
	drm_intel_bufmgr_destroy(bufmgr);
}
