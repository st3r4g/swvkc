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

	// vblank callback
	void (*vblank_notify)(int,unsigned int,unsigned int, unsigned int,
	void*);
	void *user_data;
};

int drm_setup(struct screen *);
int gbm_setup(struct screen *, bool dmabuf_mod);

struct screen *screen_setup(void (*vblank_notify)(int,unsigned int,unsigned int,
unsigned int, void*), void *user_data, bool dmabuf_mod) {
	struct screen *screen = calloc(1, sizeof(struct screen));
	screen->vblank_notify = vblank_notify;
	screen->user_data = user_data;
	drm_setup(screen);
	gbm_setup(screen, dmabuf_mod);

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

int crtc_index_from_id(int fd, uint32_t crtc_id) {
	drmModeRes *res = drmModeGetResources(fd);
	for (int i=0; i<res->count_crtcs; i++) {
		if (res->crtcs[i] == crtc_id) {
			drmModeFreeResources(res);
			return i;
		}
	}
	drmModeFreeResources(res);
	return -1;
}

/* replacement for simple_modeset */
void planes(struct screen *S, uint32_t crtc_id) {
	drmModePlaneRes *plane_res = drmModeGetPlaneResources(S->gpu_fd);
	for (size_t i=0; i<plane_res->count_planes; i++) {
		drmModePlane *plane = drmModeGetPlane(S->gpu_fd, plane_res->planes[i]);
		if (!(plane->possible_crtcs & (1 << crtc_index_from_id(S->gpu_fd, crtc_id)))) {
			drmModeFreePlane(plane);
			continue;
		}
		drmModeObjectProperties *obj_props;
		obj_props = drmModeObjectGetProperties(S->gpu_fd, plane->plane_id,
		DRM_MODE_OBJECT_PLANE);
		for (size_t j=0; j<obj_props->count_props; j++) {
			drmModePropertyRes *prop = drmModeGetProperty(S->gpu_fd,
			obj_props->props[j]);
			if (!strcmp(prop->name, "type")) {
				if (obj_props->prop_values[j] == DRM_PLANE_TYPE_PRIMARY) {
					errlog("am primary %d", plane->plane_id);
					S->plane_id = plane->plane_id;
				} else if (obj_props->prop_values[j] == DRM_PLANE_TYPE_OVERLAY) {
					errlog("am overlay %d", plane->plane_id);
					S->overlay_plane_id = plane->plane_id;
				}
			}
			drmModeFreeProperty(prop);
		}
		drmModeFreeObjectProperties(obj_props);
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

	S->crtc_id = crtc->crtc_id;
	simple_modeset(S, crtc->crtc_id); // TODO remove
	planes(S, crtc->crtc_id);
	drmModeFreeCrtc(crtc);
	S->props = find_prop_ids(S->gpu_fd, S->plane_id, connected[n]->connector_id);
	S->props_plane_fb_id = S->props.plane.fb_id;

	errlog("%d %d %d", S->plane_id, S->props_plane_fb_id, S->old_fb_id);

	request_vblank(S);
	return 0;
}

int gbm_setup(struct screen *S, bool dmabuf_mod) {
	S->gbm_device = gbm_create_device(S->gpu_fd);
	if (!S->gbm_device) {
		fprintf(stderr, "gbm_create_device failed\n");
		return 1;
	}

	uint32_t flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;
/*	if (!dmabuf_mod) XXX: otherwise GBM creates a bad bo for scanout
		flags |= GBM_BO_USE_LINEAR;*/

	int ret = gbm_device_is_format_supported(S->gbm_device,
	GBM_BO_FORMAT_XRGB8888, flags);
	if (!ret) {
		fprintf(stderr, "format unsupported\n");
		return 1;
	}

	struct box screen_size = screen_get_dimensions(S);
	S->gbm_bo = gbm_bo_create(S->gbm_device, screen_size.width,
	screen_size.height, GBM_BO_FORMAT_XRGB8888, flags);
/*	uint64_t modifier = I915_FORMAT_MOD_X_TILED;
	S->gbm_bo = gbm_bo_create_with_modifiers(S->gbm_device,
			screen_size.width, screen_size.height,
			GBM_BO_FORMAT_XRGB8888, &modifier, 1);*/
	if (!S->gbm_bo) {
		fprintf(stderr, "gbm_bo_create failed\n");
		return 1;
	}

	return 0;
}


static void vblank_handler(int fd, unsigned int sequence, unsigned int tv_sec,
unsigned int tv_usec, void *user_data) {
	struct screen *screen = user_data;
//	errlog("VBLANK %d %.3f", sequence, tv_sec * 1000 + tv_usec / 1000.0);
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

void screen_post(struct screen *S, int fence_fd) {
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
	}

//	uint32_t handle = gbm_bo_get_handle(bo).u32;
	uint32_t bo_format = gbm_bo_get_format(bo);

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
//	fprintf(stderr, "atomic commit success\n");
//	if (fb_id)
//	drmModeRmFB(S->gpu_fd, fb_id);
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

uint64_t screen_get_bo_modifier(struct screen *S) {
	return gbm_bo_get_modifier(S->gbm_bo);
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

bool screen_is_overlay_supported(struct screen *S) {
	return S->overlay_plane_id > 0;
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
		if (!strcmp(prop->name, "type"))
			props.plane.type = prop->prop_id;
		if (!strcmp(prop->name, "SRC_X"))
			props.plane.src_x = prop->prop_id;
		if (!strcmp(prop->name, "SRC_Y"))
			props.plane.src_y = prop->prop_id;
		if (!strcmp(prop->name, "SRC_W"))
			props.plane.src_w = prop->prop_id;
		if (!strcmp(prop->name, "SRC_H"))
			props.plane.src_h = prop->prop_id;
		if (!strcmp(prop->name, "CRTC_X"))
			props.plane.crtc_x = prop->prop_id;
		if (!strcmp(prop->name, "CRTC_Y"))
			props.plane.crtc_y = prop->prop_id;
		if (!strcmp(prop->name, "CRTC_W"))
			props.plane.crtc_w = prop->prop_id;
		if (!strcmp(prop->name, "CRTC_H"))
			props.plane.crtc_h = prop->prop_id;
		if (!strcmp(prop->name, "FB_ID"))
			props.plane.fb_id = prop->prop_id;
		if (!strcmp(prop->name, "IN_FENCE_FD"))
			props.plane.in_fence_fd = prop->prop_id;
		if (!strcmp(prop->name, "CRTC_ID"))
			props.plane.crtc_id = prop->prop_id;
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
