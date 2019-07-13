#define _POSIX_C_SOURCE 200809L
#include <backend/screen.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <drm_fourcc.h>
#include <gbm.h>
#include <libudev.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <util/log.h>

int flag = 0;

// GBM formats are either GBM_BO_FORMAT_XRGB8888 or GBM_BO_FORMAT_ARGB8888
//static const int COLOR_DEPTH = 24;
//static const int BIT_PER_PIXEL = 32;

struct props {
	struct {
		uint32_t fb_id;
		uint32_t crtc_id;
	} plane;

	struct {
		uint32_t crtc_id;
	} conn;
};

struct screen {
	// DRM
	int gpu_fd;
	uint32_t plane_id;
	uint32_t props_plane_fb_id;
	uint32_t old_fb_id;
	uint32_t fb_id[2];
	drmModeAtomicReq *req;

	// GBM
	struct gbm_device *gbm_device;
	struct gbm_surface *gbm_surface;
	struct gbm_bo *gbm_bo;

	// vblank callback
	void (*vblank_notify)(int,unsigned int,unsigned int, unsigned int,
	void*);
	void *user_data;
};

int drm_setup(struct screen *);
int gbm_setup(struct screen *);

struct screen *screen_setup(void (*vblank_notify)(int,unsigned int,unsigned int,
unsigned int, void*), void *user_data) {
	struct screen *screen = calloc(1, sizeof(struct screen));
	screen->vblank_notify = vblank_notify;
	screen->user_data = user_data;
	drm_setup(screen);
	gbm_setup(screen);

	return screen;
}

char *boot_gpu_devpath() {
	char *devpath;
	struct udev *udev = udev_new();
	struct udev_enumerate *enu = udev_enumerate_new(udev);
	udev_enumerate_add_match_sysattr(enu, "boot_vga", "1");
	udev_enumerate_scan_devices(enu);
	struct udev_list_entry *cur;
	udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
		struct udev_device *dev = udev_device_new_from_syspath(udev,
		udev_list_entry_get_name(cur));
		udev_enumerate_unref(enu);
		enu = udev_enumerate_new(udev);
		udev_enumerate_add_match_parent(enu, dev);
		udev_enumerate_add_match_sysname(enu, "card[0-9]");
		udev_enumerate_scan_devices(enu);
		udev_device_unref(dev);
		udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
			dev = udev_device_new_from_syspath(udev,
			udev_list_entry_get_name(cur));
			const char *str = udev_device_get_devnode(dev);
			devpath = malloc((strlen(str)+1)*sizeof(char));
			strcpy(devpath, str);
			udev_device_unref(dev);
			udev_enumerate_unref(enu);
			break;
		}
		break;
	}
	return devpath;
}

void simple_modeset(struct screen *S, uint32_t crtc_id) {
	drmModePlaneRes *plane_res = drmModeGetPlaneResources(S->gpu_fd);
	for (size_t i=0; i<plane_res->count_planes; i++) {
		drmModePlane *plane = drmModeGetPlane(S->gpu_fd, plane_res->planes[i]);
		if (plane->crtc_id == crtc_id) {
			S->plane_id = plane->plane_id;
			S->old_fb_id = plane->fb_id;
		}
		drmModeFreePlane(plane);
	}
	drmModeFreePlaneResources(plane_res);
}

static int modeset_find_crtc(int fd, drmModeRes *res, drmModeConnector *conn);
struct props find_prop_ids(int fd, uint32_t plane_id, uint32_t conn_id);

drmModeCrtc *get_crtc_from_conn(int fd, drmModeConnector *conn) {
	if (conn->encoder_id) {
		drmModeEncoder *enc = drmModeGetEncoder(fd, conn->encoder_id);
		drmModeCrtc *crtc = drmModeGetCrtc(fd, enc->crtc_id);
		drmModeFreeEncoder(enc);
		return crtc;
	} else
		return 0;
}

void request_vblank(struct screen *screen) {
	drmVBlank vbl = {
		.request.type = DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT,
		.request.sequence = 1,
		.request.signal = (unsigned long)screen
	};
	if(drmWaitVBlank(screen->gpu_fd, &vbl) != 0) {
		fprintf(stderr, "drmWaitVBlank (relative) failed\n");
	}
}

int drm_setup(struct screen *S) {
	char *devpath = boot_gpu_devpath();
	S->gpu_fd = open(devpath, O_RDWR|O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
	if (S->gpu_fd < 0) {
		perror("open /dev/dri/card0");
		return 1;
	}
	free(devpath);

	if (!drmIsMaster(S->gpu_fd)) {
		fprintf(stderr, "The display is already taken by another program\n");
		exit(1);
	}

	if(drmSetClientCap(S->gpu_fd, DRM_CLIENT_CAP_ATOMIC, 1)) {
		fprintf(stderr, "ATOMIC MODESETTING UNSUPPORTED :C\n");
		return 1;
	}
	
	drmModeRes *res = drmModeGetResources(S->gpu_fd);
	int count = 0;
	for (int i=0; i<res->count_connectors; i++) {
		drmModeConnector *conn = drmModeGetConnector(S->gpu_fd,
		res->connectors[i]);
		if (conn->connection == DRM_MODE_CONNECTED) {
			count++;
		}
		drmModeFreeConnector(conn);
	}
	drmModeConnector **connected = malloc(count*sizeof(drmModeConnector));
	for (int i=0,j=0; i<res->count_connectors; i++) {
		drmModeConnector *conn = drmModeGetConnector(S->gpu_fd,
		res->connectors[i]);
		if (conn->connection == DRM_MODE_CONNECTED) {
			connected[j] = conn;
			j++;
		} else
			drmModeFreeConnector(conn);
	}
	drmModeFreeResources(res);
	int n;
	if (count > 1) {
		printf("Found multiple displays:\n");
		for (int i=0; i<count; i++) {
			printf("(%d) [%d]\n", i, connected[i]->connector_id);
		}
		printf("Choose one: ");
		scanf("%d", &n);
	} else {
		n = 0;
	}
	drmModeCrtc *crtc = get_crtc_from_conn(S->gpu_fd, connected[n]);
	errlog("vrefresh: %d, connected: %d\n", crtc->mode.vrefresh,
	connected[n]->connector_id);

	simple_modeset(S, crtc->crtc_id);
	drmModeFreeCrtc(crtc);
	struct props props = find_prop_ids(S->gpu_fd, S->plane_id, connected[n]->connector_id);
	S->props_plane_fb_id = props.plane.fb_id;

	errlog("%d %d %d", S->plane_id, S->props_plane_fb_id, S->old_fb_id);

	request_vblank(S);
	return 0;
}

int gbm_setup(struct screen *S) {
	S->gbm_device = gbm_create_device(S->gpu_fd);
	if (!S->gbm_device) {
		fprintf(stderr, "gbm_create_device failed\n");
		return 1;
	}

	int ret = gbm_device_is_format_supported(S->gbm_device,
	GBM_BO_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING | GBM_BO_USE_LINEAR);
	if (!ret) {
		fprintf(stderr, "format unsupported\n");
		return 1;
	}

	uint32_t flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING | GBM_BO_USE_LINEAR;
	struct box screen_size = screen_get_dimensions(S);
	S->gbm_bo = gbm_bo_create(S->gbm_device, screen_size.width,
	screen_size.height, GBM_BO_FORMAT_XRGB8888, flags);
	if (!S->gbm_bo) {
		fprintf(stderr, "gbm_bo_create failed\n");
		return 1;
	}

	return 0;
}


static void vblank_handler(int fd, unsigned int sequence, unsigned int tv_sec,
unsigned int tv_usec, void *user_data) {
	struct screen *screen = user_data;
	errlog("VBLANK %d %.3f", sequence, tv_sec * 1000 + tv_usec / 1000.0);
	// TODO: better
	if (flag == 0)
		screen->vblank_notify(fd, sequence, tv_sec, tv_usec, screen->user_data);
	request_vblank(screen);
}

static void page_flip_handler(int fd, unsigned int sequence, unsigned int
tv_sec, unsigned int tv_usec, void *user_data) {
	struct screen *screen = user_data;
	flag = 0;
	screen->vblank_notify(fd, sequence, tv_sec, tv_usec, screen->user_data);
}

void drm_handle_event(int fd) {
	drmEventContext ev_context = {
		.version = DRM_EVENT_CONTEXT_VERSION,
		.vblank_handler = vblank_handler,
		.page_flip_handler = page_flip_handler,
	};
	drmHandleEvent(fd, &ev_context);
}

void screen_post_direct(struct screen *S, uint32_t width, uint32_t height,
uint32_t format, int fd, int stride, int offset, uint64_t modifier) {
	errlog("flag %d", flag);
	flag = 1;
	static int i = 0;
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	if (!req)
		fprintf(stderr, "atomic allocation failed\n");

	errlog("%d %d %d %d %d %d %d", width, height, format, fd, stride, offset, modifier);
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
	struct gbm_bo *bo = gbm_bo_import(S->gbm_device,
	GBM_BO_IMPORT_FD_MODIFIER, &buffer, GBM_BO_USE_SCANOUT);

	if (!bo) {
		perror("import");
	}

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

void screen_post(struct screen *S) {
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	struct gbm_bo *bo = S->gbm_bo;
	uint32_t bo_width = gbm_bo_get_width(bo);
	uint32_t bo_height = gbm_bo_get_height(bo);

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
		errlog("ciccioc %d", bo_modifiers[j] == I915_FORMAT_MOD_X_TILED);
	}

//	uint32_t handle = gbm_bo_get_handle(bo).u32;
	uint32_t bo_format = gbm_bo_get_format(bo);
		errlog("%d %d %d", bo_format, DRM_FORMAT_XRGB8888, DRM_FORMAT_ARGB8888);

/*	if (drmModeAddFB2WithModifiers(S->gpu_fd, width, height, COLOR_DEPTH, BIT_PER_PIXEL,
	stride, handle, &S->fb_id[i])) {*/
	uint32_t fb_id;
	if (drmModeAddFB2WithModifiers(S->gpu_fd, bo_width, bo_height, bo_format,
	bo_handles, bo_strides, bo_offsets, bo_modifiers, &fb_id, DRM_MODE_FB_MODIFIERS)) {
		perror("AddFB");
	}

	if (drmModeAtomicAddProperty(req, S->plane_id,
	S->props_plane_fb_id, fb_id) < 0)
		fprintf(stderr, "atomic add property failed\n");
	if (drmModeAtomicCommit(S->gpu_fd, req, DRM_MODE_PAGE_FLIP_EVENT |
	DRM_MODE_ATOMIC_NONBLOCK, S))
	perror("atomic commit failed");
	else {
	fprintf(stderr, "atomic commit success\n");
/*	if (fb_id)
	drmModeRmFB(S->gpu_fd, fb_id);*/
	}
	drmModeAtomicFree(req);
}

int screen_get_gpu_fd(struct screen *S) {
	return S->gpu_fd;
}

int screen_get_bo_fd(struct screen *S) {
	return gbm_bo_get_fd(S->gbm_bo);
}

uint32_t screen_get_bo_stride(struct screen *S) {
	return gbm_bo_get_stride(S->gbm_bo);
}

struct gbm_device *screen_get_gbm_device(struct screen *S) {
	return S->gbm_device;
}

struct box screen_get_dimensions(struct screen *S) {
	struct box box;
	drmModeFB *old_fb = drmModeGetFB(S->gpu_fd, S->old_fb_id);
	box.width = old_fb->width, box.height = old_fb->height;
	drmModeFreeFB(old_fb);
	return box;
}

void screen_release(struct screen *S) {
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	if (drmModeAtomicAddProperty(req, S->plane_id, S->props_plane_fb_id, S->old_fb_id) < 0)
		fprintf(stderr, "atomic add property failed\n");
	if(drmModeAtomicCommit(S->gpu_fd, req, 0, 0))
		perror("atomic commit failed");
	
	drmModeAtomicFree(req);
//	drmModeRmFB(S->gpu_fd, S->fb.id);
	close(S->gpu_fd);
	free(S);
}

/*
 * from drm-kms(7)
 */
static int modeset_find_crtc(int fd, drmModeRes *res, drmModeConnector *conn) {
	drmModeEncoder *enc;

	/* iterate all encoders of this connector */
	for (int i = 0; i < conn->count_encoders; ++i) {
		enc = drmModeGetEncoder(fd, conn->encoders[i]);
		if (!enc) {
			/* cannot retrieve encoder, ignoring... */
			continue;
		}

		/* iterate all global CRTCs */
		for (int j = 0; j < res->count_crtcs; ++j) {
			/* check whether this CRTC works with the encoder */
			if (!(enc->possible_crtcs & (1 << j)))
				continue;

			/* Here you need to check that no other connector
			* currently uses the CRTC with id "crtc". If you intend
			* to drive one connector only, then you can skip this
			* step. Otherwise, simply scan your list of configured
			* connectors and CRTCs whether this CRTC is already
			* used. If it is, then simply continue the search here. */
			drmModeFreeEncoder(enc);
			return res->crtcs[j];
		}
		drmModeFreeEncoder(enc);
	}
	/* cannot find a suitable CRTC */
	return -1;
}

struct props find_prop_ids(int fd, uint32_t plane_id, uint32_t conn_id) {
	struct props props;
	drmModeObjectProperties *obj_props;
	obj_props = drmModeObjectGetProperties(fd, plane_id,
	DRM_MODE_OBJECT_PLANE);
	for (size_t i=0; i<obj_props->count_props; i++) {
		drmModePropertyRes *prop = drmModeGetProperty(fd,
		obj_props->props[i]);
		if (!strcmp(prop->name, "FB_ID"))
			props.plane.fb_id = prop->prop_id;
		if (!strcmp(prop->name, "CRTC_ID"))
			props.plane.crtc_id = prop->prop_id;
		if (!strcmp(prop->name, "SRC_W"))
			errlog("SRC_W: %d %"PRIu64"", prop->prop_id,
			obj_props->prop_values[i] >> 16);
		if (!strcmp(prop->name, "SRC_H"))
			errlog("SRC_W: %d %"PRIu64"", prop->prop_id,
			obj_props->prop_values[i] >> 16);
		if (!strcmp(prop->name, "CRTC_W"))
			errlog("CRTC_W: %d %"PRIu64"", prop->prop_id,
			obj_props->prop_values[i]);
		if (!strcmp(prop->name, "CRTC_H"))
			errlog("CRTC_H: %d %"PRIu64"", prop->prop_id,
			obj_props->prop_values[i]);
		drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(obj_props);
	obj_props = drmModeObjectGetProperties(fd, conn_id,
	DRM_MODE_OBJECT_CONNECTOR);
	for (size_t i=0; i<obj_props->count_props; i++) {
		drmModePropertyRes *prop = drmModeGetProperty(fd,
		obj_props->props[i]);
		if (!strcmp(prop->name, "CRTC_ID"))
			props.conn.crtc_id = prop->prop_id;
		drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(obj_props);
	return props;
}
