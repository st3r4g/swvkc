#include <libdrm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <skalibs/strerr.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define die(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

static uint64_t find_old(int fd) {
	drmModeObjectPropertiesPtr ptr = drmModeObjectGetProperties(fd, 40, DRM_MODE_OBJECT_PLANE);
	for (int i=0; i<ptr->count_props; i++) {
		drmModePropertyPtr prop = drmModeGetProperty(fd, ptr->props[i]);
		if (!strcmp(prop->name, "FB_ID"))
			return ptr->prop_values[i];
			//printf("%s %d %u\n", prop->name, prop->prop_id, ptr->prop_values[i]);
		drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(ptr);
	return 0;
}

#include "modeset.h"

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

struct props find_prop_ids(int fd, uint32_t plane_id, uint32_t conn_id, uint32_t crtc_id) {
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
	obj_props = drmModeObjectGetProperties(fd, crtc_id,
	DRM_MODE_OBJECT_CRTC);
	for (size_t i=0; i<obj_props->count_props; i++) {
		drmModePropertyRes *prop = drmModeGetProperty(fd,
		obj_props->props[i]);
		if (!strcmp(prop->name, "OUT_FENCE_PTR"))
			props.crtc.out_fence_ptr = prop->prop_id;
		drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(obj_props);

	return props;
}

static int crtc_index_from_id(int fd, uint32_t crtc_id) {
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

static uint32_t planes(int fd, uint32_t crtc_id) {
	int crtc_idx = crtc_index_from_id(fd, crtc_id);
	int n_overlay = 0, n_cursor = 0;
	uint32_t plane_id = 0;

	drmModePlaneRes *plane_res = drmModeGetPlaneResources(fd);
	for (size_t i=0; i<plane_res->count_planes; i++) {
		drmModePlane *plane = drmModeGetPlane(fd, plane_res->planes[i]);
		if (!(plane->possible_crtcs & (1 << crtc_idx))) {
			drmModeFreePlane(plane);
			continue;
		}
		drmModeObjectProperties *obj_props;
		obj_props = drmModeObjectGetProperties(fd, plane->plane_id,
		DRM_MODE_OBJECT_PLANE);
		for (size_t j=0; j<obj_props->count_props; j++) {
			drmModePropertyRes *prop = drmModeGetProperty(fd,
			obj_props->props[j]);
			if (!strcmp(prop->name, "type")) {
				if (obj_props->prop_values[j] == DRM_PLANE_TYPE_PRIMARY) {
					printf("primary %d\n", plane->plane_id);
					plane_id = plane->plane_id;
				} else if (obj_props->prop_values[j] == DRM_PLANE_TYPE_OVERLAY) {
					printf("overlay %d\n", plane->plane_id);
					//S->overlay_plane_id = plane->plane_id;
					n_overlay++;
				} else if (obj_props->prop_values[j] == DRM_PLANE_TYPE_CURSOR) {
					printf("cursor %d\n", plane->plane_id);
					//S->cursor_plane_id = plane->plane_id;
					n_cursor++;
				}
			}
			drmModeFreeProperty(prop);
		}
		drmModeFreeObjectProperties(obj_props);
		drmModeFreePlane(plane);
	}
	drmModeFreePlaneResources(plane_res);
	//boxlog("   overlay planes: %d", n_overlay);
	//boxlog("    cursor planes: %d", n_cursor);
	return plane_id;
}

static uint32_t crtc;
static uint32_t conn;
static uint32_t plane;
static struct props props;

void atomic_init() {
	int r, fd = modeset_get_fd();

	// without this we don't have access to all properties
	r = drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1);
	assert(!r);

	crtc = modeset_get_crtc();
	conn = modeset_get_conn();
	plane = planes(fd, crtc);
	assert(plane);

	props = find_prop_ids(fd, plane, conn, crtc);
}

uint32_t modeset_add_fb(uint32_t width, uint32_t height, uint32_t format, uint32_t handle, uint32_t pitch, uint32_t offset, uint64_t mod) {
	int r, fd = modeset_get_fd();
	uint32_t bo_handles[4] = {handle}, pitches[4] = {pitch}, offsets[4] = {offset};
	uint64_t modifier[4] = {mod};
	uint32_t buf_id, flags = DRM_MODE_FB_MODIFIERS;

	// remove alpha otherwise fails on my (intel) gpu
	format = format == DRM_FORMAT_ARGB8888 ? DRM_FORMAT_XRGB8888 : format;

	r = drmModeAddFB2WithModifiers(fd, width, height, format, bo_handles,
	                           pitches, offsets, modifier, &buf_id, flags);
	if (r != 0) die("drmModeAddFB2WithModifiers");

	printf("success: %dx%d\n", width, height);
	return buf_id;
}

void modeset_rem_fb(uint32_t buf_id) {
	int r, fd = modeset_get_fd();

	r = drmModeRmFB(fd, buf_id);
	if (r != 0) strerr_warn1sys("drmModeAddFB2WithModifiers");
}

void atomic_commit(uint32_t buf_id) {
	int r, fd = modeset_get_fd();
//	if (width != 24) {
		drmModeAtomicReqPtr req = drmModeAtomicAlloc();
		if (req == NULL) die("drmModeAtomicAlloc");
		drmModeAtomicAddProperty(req, plane, props.plane.fb_id, buf_id);
		r = drmModeAtomicCommit(fd, req, DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK, NULL);
		if (r != 0) die("drmModeAtomicCommit");
		drmModeAtomicFree(req);
//	}

	/*uint64_t old_buf_id = find_old(fd);
	sleep(1);

	req = drmModeAtomicAlloc();
	if (req == NULL) die("drmModeAtomicAlloc");
	drmModeAtomicAddProperty(req, 40, 16, old_buf_id);
	r = drmModeAtomicCommit(fd, req, 0, NULL);
	if (r != 0) die("drmModeAtomicCommit");
	drmModeAtomicFree(req);*/
}

#include "wayland.h" // meh

void drm_read() {
	int fd = modeset_get_fd();
	drmEventContext ev_context = {
		.version = DRM_EVENT_CONTEXT_VERSION,
		.page_flip_handler = page_flip_handler,
	};
	drmHandleEvent(fd, &ev_context);
}
