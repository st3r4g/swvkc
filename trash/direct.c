#define _POSIX_C_SOURCE 200809L

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <backend/bufmgr.h>
#include <backend/screen.h>
#include <util/log.h>

// XXX temp duplicate am lazy XXX
/* A bit wrong because different planes could have different properties */
struct props {
	struct {
		uint32_t type;
		uint32_t src_x;
		uint32_t src_y;
		uint32_t src_w;
		uint32_t src_h;
		uint32_t crtc_x;
		uint32_t crtc_y;
		uint32_t crtc_w;
		uint32_t crtc_h;
		uint32_t fb_id;
		uint32_t in_fence_fd;
		uint32_t crtc_id;
	} plane;

	struct {
		uint32_t crtc_id;
	} conn;

	struct {
		uint32_t out_fence_ptr;
	} crtc;
};

struct screen {
	// DRM
	int gpu_fd;
	uint32_t plane_id;
	uint32_t overlay_plane_id;
	uint32_t crtc_id;
	uint32_t props_plane_fb_id;
	uint32_t old_fb_id;
	uint32_t fb_id[2];
	drmModeAtomicReq *req;

	struct props props;

	// GBM
	struct gbm_device *gbm_device;
	struct gbm_surface *gbm_surface;
	struct gbm_bo *gbm_bo;

	struct bufmgr *bufmgr;

	// vblank callback
	void (*vblank_notify)(int,unsigned int,unsigned int, unsigned int,
	void*, bool);
	void *user_data;
	// out_fence callback
	void (*listen_to_out_fence)(int, void*);
	void *user_data2;

	bool vblank_has_page_flip;
};

void screen_post_direct(struct screen *S, uint32_t width, uint32_t height,
uint32_t format, int fd, int stride, int offset, uint64_t modifier) {
	static int i = 0;
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	if (!req)
		fprintf(stderr, "atomic allocation failed\n");

	errlog("%d %d %d %d %d %d %d", width, height, format, fd, stride, offset, modifier);

	uint32_t bo_width = gbm_bo_get_width(bo);
	uint32_t bo_height = gbm_bo_get_height(bo);
		errlog("%d", bo_width);
		errlog("%d", bo_height);

	uint32_t bo_handles[4] = {0};
	uint32_t bo_strides[4] = {0};
	uint32_t bo_offsets[4] = {0};
	uint64_t bo_modifiers[4] = {0};
	for (int j = 0; j < gbm_bo_get_plane_count(bo); j++) {
		bo_handles[j] = gbm_bo_get_handle_for_plane(bo, j).u32;
		bo_strides[j] = gbm_bo_get_stride_for_plane(bo, j);
		bo_offsets[j] = gbm_bo_get_offset(bo, j);
		// KMS requires all BO planes to have the same modifier
		bo_modifiers[j] = gbm_bo_get_modifier(bo);
		errlog("%d", bo_handles[j]);
		errlog("%d", bo_strides[j]);
		errlog("%d", bo_offsets[j]);
		errlog("ciccioc %d", bo_modifiers[j] == I915_FORMAT_MOD_Y_TILED);
	}

//	uint32_t handle = gbm_bo_get_handle(bo).u32;
	uint32_t bo_format = format;
		errlog("%d %d %d", bo_format, DRM_FORMAT_XRGB8888, DRM_FORMAT_ARGB8888);

/*	if (drmModeAddFB2WithModifiers(S->gpu_fd, width, height, COLOR_DEPTH, BIT_PER_PIXEL,
	stride, handle, &S->fb_id[i])) {*/
	if (drmModeAddFB2WithModifiers(S->gpu_fd, bo_width, bo_height, bo_format,
	bo_handles, bo_strides, bo_offsets, bo_modifiers, &S->fb_id[i], DRM_MODE_FB_MODIFIERS)) {
		perror("AddFB");
	}

	printf("%d %d\n", S->fb_id[i], S->fb_id[!i]);

	if (drmModeAtomicAddProperty(req, S->plane_id,
	S->props_plane_fb_id, S->fb_id[i]) < 0)
		fprintf(stderr, "atomic add property failed\n");
//	drmModeAtomicAddProperty(req, S->plane_id, 10, 1366 << 16); //W
//	drmModeAtomicAddProperty(req, S->plane_id, 11, 768 << 16); //H
//	drmModeAtomicAddProperty(req, S->plane_id, 12, 10); //W
//	drmModeAtomicAddProperty(req, S->plane_id, 13, 10); //H
//	drmModeAtomicAddProperty(req, S->plane_id, 14, 1366); //W
//	drmModeAtomicAddProperty(req, S->plane_id, 15, 768); //H

	errlog("Trying to post the new buffer");
	if (drmModeAtomicCommit(S->gpu_fd, req, DRM_MODE_ATOMIC_TEST_ONLY |
	DRM_MODE_ATOMIC_ALLOW_MODESET, 0))
	perror("test failed");
	else {
	fprintf(stderr, "test success\n");
	}
	if (drmModeAtomicCommit(S->gpu_fd, req, DRM_MODE_PAGE_FLIP_EVENT |
	DRM_MODE_ATOMIC_NONBLOCK, S))
	perror("atomic commit failed");
	else {
	fprintf(stderr, "atomic commit success\n");
	if (S->fb_id[!i])
	drmModeRmFB(S->gpu_fd, S->fb_id[!i]);
	}
	drmModeAtomicFree(req);
	errlog("Trying to post the new buffer 2");
//	errlog("B");
//	errlog("C");
	i = !i;
}

uint32_t fb_id[2] = {0};
struct gbm_bo *bo[2] = {0};

/*void import_dmabuf_with_gbm() {
	struct gbm_import_fd_modifier_data buffer = {
		.width = width,
		.height = height,
		.format = format,
		.num_fds = 1,
		.fds[0] = fd,
		.strides[0] = stride,
		.offsets[0] = offset,
		.modifier = modifier
	};
	bo[i] = gbm_bo_import(S->gbm_device, GBM_BO_IMPORT_FD_MODIFIER, &buffer,
	 GBM_BO_USE_SCANOUT);

	if (!bo[i]) {
		perror("import");
	}

	uint32_t bo_width = gbm_bo_get_width(bo[i]);
	uint32_t bo_height = gbm_bo_get_height(bo[i]);

	uint32_t bo_handles[4] = {0};
	uint32_t bo_strides[4] = {0};
	uint32_t bo_offsets[4] = {0};
	uint64_t bo_modifiers[4] = {0};
//	errlog("number of planes: %d", gbm_bo_get_plane_count(bo));
	for (int j = 0; j < 1; j++) {
		bo_handles[j] = gbm_bo_get_handle_for_plane(bo[i], j).u32;
//		errlog("bo handle %d: %d", j, bo_handles[j]);
		bo_strides[j] = gbm_bo_get_stride_for_plane(bo[i], j);
		bo_offsets[j] = gbm_bo_get_offset(bo[i], j);
		// KMS requires all BO planes to have the same modifier
		bo_modifiers[j] = gbm_bo_get_modifier(bo[i]);
	}

//	uint32_t handle = gbm_bo_get_handle(bo).u32;
	uint32_t bo_format = gbm_bo_get_format(bo[i]);
}*/


