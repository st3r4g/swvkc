#define _POSIX_C_SOURCE 200809L
#include <backend/screen.h>

#include <wayland-util.h> // just for wl_list

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <backend/bufmgr.h>
#include <backend/dev.h>

#include <util/log.h>
#include <util/util.h>
#include <util/my_drm_handle_event.h>

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

	struct {
		uint32_t out_fence_ptr;
	} crtc;
};

struct fb {
	uint32_t id;
	struct buffer *buffer;
};

struct fb_node {
	struct fb *fb;
	struct wl_list link;
};

struct fb *cursor; //TODO: remove

struct screen {
	// DRM
	int gpu_fd;
	uint32_t plane_id;
	uint32_t overlay_plane_id;
	uint32_t cursor_plane_id;
	uint32_t crtc_id;
	uint32_t props_plane_fb_id;
	uint32_t old_fb_id;
	uint32_t fb_id[2];
	drmModeAtomicReq *req;

	struct props props;

	struct bufmgr *bufmgr;
	struct fb *back; // The main framebuffer -- back
	struct fb *front; // The main framebuffer -- front
	struct fb *cursor;

	struct wl_list fb_destroy_list;

	// vblank callback
	void (*vblank_notify)(int,unsigned int,unsigned int, unsigned int,
	void*, bool);
	void *user_data;

	bool vblank_has_page_flip;
	bool pending_page_flip;
};

int drm_setup(struct screen *);
struct fb *screen_fb_create_main(struct screen *screen, int width, int height,
bool linear, bool no_alpha);
void screen_fb_destroy(struct screen *screen, struct fb *fb);
const char *conn_get_name(uint32_t type_id);

struct screen *screen_setup(void (*vblank_notify)(int,unsigned int,unsigned int,
unsigned int, void*, bool), void *user_data, bool dmabuf_mod) {
	printf("├─ VIDEO (Direct Rendering Manager)\n");
	struct screen *screen = calloc(1, sizeof(struct screen));
	wl_list_init(&screen->fb_destroy_list);
	screen->vblank_notify = vblank_notify;
	screen->user_data = user_data;

	if (drm_setup(screen) < 0) {
		return NULL;
	}

	screen->bufmgr = bufmgr_create(screen->gpu_fd);
	struct box screen_size = screen_get_dimensions(screen);
	screen->back = screen_fb_create_main(screen, screen_size.width,
	 screen_size.height, !dmabuf_mod, true);
	screen->front = screen_fb_create_main(screen, screen_size.width,
	 screen_size.height, !dmabuf_mod, true);
	screen->cursor = screen_fb_create_main(screen, 64, 64, true,
	 false);
	cursor = screen->cursor; // TODO remove

	return screen;
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
	int n_overlay = 0, n_cursor = 0;
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
//					boxlog("primary %d", plane->plane_id);
					S->plane_id = plane->plane_id;
				} else if (obj_props->prop_values[j] == DRM_PLANE_TYPE_OVERLAY) {
//					boxlog("overlay %d", plane->plane_id);
					S->overlay_plane_id = plane->plane_id;
					n_overlay++;
				} else if (obj_props->prop_values[j] == DRM_PLANE_TYPE_CURSOR) {
					S->cursor_plane_id = plane->plane_id;
					n_cursor++;
				}
			}
			drmModeFreeProperty(prop);
		}
		drmModeFreeObjectProperties(obj_props);
		drmModeFreePlane(plane);
	}
	drmModeFreePlaneResources(plane_res);
	boxlog("   overlay planes: %d", n_overlay);
	boxlog("    cursor planes: %d", n_cursor);
}

static int modeset_find_crtc(int fd, drmModeRes *res, drmModeConnector *conn);
struct props find_prop_ids(int fd, uint32_t plane_id, uint32_t conn_id, uint32_t crtc_id);

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
static int read_cache(uint32_t *connector_id, uint32_t *crtc_id) {
	FILE *f1 = fopen("/home/stefano/.cache/swvkc/connector_id", "r");
	FILE *f2 = fopen("/home/stefano/.cache/swvkc/crtc_id", "r");
	if (!f1 || !f2)
		return -1;
	char str[8];
	fgets(str, 8, f1);
	*connector_id = strtoul(str, NULL, 10);
	fgets(str, 8, f2);
	*crtc_id = strtoul(str, NULL, 10);
	fclose(f1);
	fclose(f2);
	return 0;
}

int drm_setup(struct screen *S) {
	char *devpath = boot_gpu_devpath();
	if (!devpath) {
		fprintf(stderr, "No suitable DRM device found");
		return -1;
	}
	S->gpu_fd = open(devpath, O_RDWR | O_CLOEXEC);
	if (S->gpu_fd < 0) {
		fprintf(stderr, "open %s: %s\n", devpath, strerror(errno));
		return -1;
	}
	free(devpath);

	if (!drmIsMaster(S->gpu_fd)) {
		fprintf(stderr, "The display is already taken by another program\n");
		exit(1);
	}

	if(drmSetClientCap(S->gpu_fd, DRM_CLIENT_CAP_ATOMIC, 1)) {
		fprintf(stderr, "ATOMIC MODESETTING UNSUPPORTED :C\n");
		return -1;
	}
	
	uint32_t x1, x2;
	if (read_cache(&x1, &x2) < 0) {
		drmModeRes *res = drmModeGetResources(S->gpu_fd);
		drmModeConnector **connected =
		 malloc(res->count_connectors*sizeof(drmModeConnector));
		int count = 0;
		for (int i=0; i<res->count_connectors; i++) {
	/* NOTE: This function takes a lot of time (>100ms) for some connectors, biggest
	 * contributor to startup time. */
			drmModeConnector *conn = drmModeGetConnector(S->gpu_fd,
			res->connectors[i]);
			if (conn->connection == DRM_MODE_CONNECTED) {
				connected[count] = conn;
				count++;
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
		boxlog("Display connector: %s", conn_get_name(connected[n]->connector_type));
		boxlog("             mode: %s@%dHz", crtc->mode.name, crtc->mode.vrefresh);
	//	boxlog("vrefresh: %d, connected: %d", crtc->mode.vrefresh,
	//	connected[n]->connector_id);
		x1 = connected[n]->connector_id;
		x2 = crtc->crtc_id;
		drmModeFreeCrtc(crtc);
	}

	S->crtc_id = x2;
	simple_modeset(S, S->crtc_id); // TODO remove
	planes(S, S->crtc_id);
	S->props = find_prop_ids(S->gpu_fd, S->plane_id, x1, S->crtc_id);
	S->props_plane_fb_id = S->props.plane.fb_id;

//	boxlog("%d %d %d", S->plane_id, S->props_plane_fb_id, S->old_fb_id);

	alloc(S);
	request_vblank(S);
	return 0;
}

static void vblank_handler(int fd, unsigned int sequence, unsigned int tv_sec,
unsigned int tv_usec, void *user_data) {
	struct screen *screen = user_data;
	screen->vblank_notify(fd, sequence, tv_sec, tv_usec, screen->user_data,
	 screen->vblank_has_page_flip);
	request_vblank(screen);
/*
 * Reset the vblank_has_page_flip flag
 */
	screen->vblank_has_page_flip = false;
}

uint32_t get_active_fb(int fd, uint32_t plane_id);

static void page_flip_handler(int fd, unsigned int sequence, unsigned int
tv_sec, unsigned int tv_usec, void *user_data) {
	struct screen *screen = user_data;
	uint32_t active_fb_primary = get_active_fb(screen->gpu_fd, screen->plane_id);
	uint32_t active_fb_overlay = get_active_fb(screen->gpu_fd, screen->overlay_plane_id);

	struct fb_node *fb_node, *tmp;
	wl_list_for_each_safe(fb_node, tmp, &screen->fb_destroy_list, link) {
		if (fb_node->fb->id != active_fb_primary &&
		    fb_node->fb->id != active_fb_overlay) {
			screen_fb_destroy(screen, fb_node->fb);
			wl_list_remove(&fb_node->link);
			free(fb_node);
		}
	}

	screen->pending_page_flip = false;
	screen->vblank_has_page_flip = true;
}

void drm_handle_event(int fd) {
	drmEventContext ev_context = {
		.version = DRM_EVENT_CONTEXT_VERSION,
		.vblank_handler = vblank_handler,
		.page_flip_handler = page_flip_handler,
	};
/*
 * Function modified so that page_flip_handler gets called before vblank_handler
 */
	my_drmHandleEvent(fd, &ev_context);
}

void screen_atomic_test(struct screen *S) {
	if (drmModeAtomicCommit(S->gpu_fd, S->req, DRM_MODE_ATOMIC_TEST_ONLY |
	DRM_MODE_ATOMIC_ALLOW_MODESET, 0))
	perror("test failed");
	else {
//	fprintf(stderr, "test success\n");
	}
}

int iffdtcn = 0;
int iffdtc[64] = {-1};

int screen_atomic_commit(struct screen *self, bool with_out_fence, int
*out_fence_fd) {
	int ret;

	assert(self->req);

	if (with_out_fence)
		drmModeAtomicAddProperty(self->req, self->crtc_id,
		self->props.crtc.out_fence_ptr, (uint64_t)out_fence_fd);

	ret = drmModeAtomicCommit(self->gpu_fd, self->req,
	DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK, self);
	if (ret)
		perror("drmModeAtomicCommit");

	drmModeAtomicFree(self->req);
	self->req = drmModeAtomicAlloc();
	if (!self->req)
		fprintf(stderr, "atomic allocation failed\n");

	if (ret == 0)
		self->pending_page_flip = true;
/*
 * Close in_fence_fds to close
 */
	for (int i=0; i<iffdtcn; i++)
		close(iffdtc[i]);
	iffdtcn = 0;

	return ret;
}

bool screen_page_flip_is_pending(struct screen *self) {
	return self->pending_page_flip;
}

void alloc(struct screen *S) {
	assert(!S->req);

	S->req = drmModeAtomicAlloc();
	if (!S->req)
		fprintf(stderr, "atomic allocation failed\n");
}

void cursor_on_cursor(struct screen *S, int x, int y) {
	assert(S->req);

	drmModeAtomicAddProperty(S->req, S->cursor_plane_id, S->props.plane.src_x, 0 << 16); // SRC_X
	drmModeAtomicAddProperty(S->req, S->cursor_plane_id, S->props.plane.src_y, 0 << 16); // SRC_Y
	drmModeAtomicAddProperty(S->req, S->cursor_plane_id, S->props.plane.src_w, 64 << 16); // SRC_W
	drmModeAtomicAddProperty(S->req, S->cursor_plane_id, S->props.plane.src_h, 64 << 16); // SRC_H
	drmModeAtomicAddProperty(S->req, S->cursor_plane_id, S->props.plane.crtc_x, x); // CRTC_X
	drmModeAtomicAddProperty(S->req, S->cursor_plane_id, S->props.plane.crtc_y, y); // CRTC_Y
	drmModeAtomicAddProperty(S->req, S->cursor_plane_id, S->props.plane.crtc_w, 64); // CRTC_W
	drmModeAtomicAddProperty(S->req, S->cursor_plane_id, S->props.plane.crtc_h, 64); // CRTC_H
	drmModeAtomicAddProperty(S->req, S->cursor_plane_id, S->props.plane.crtc_id, S->crtc_id); // CRTC_ID

	if (drmModeAtomicAddProperty(S->req, S->cursor_plane_id, S->props_plane_fb_id, cursor->id) < 0)
		fprintf(stderr, "atomic add property failed\n");
}

void client_buffer_on_primary(struct screen *S, struct fb *fb, int in_fence_fd) {
	assert(S->req);

	if (drmModeAtomicAddProperty(S->req, S->plane_id, S->props_plane_fb_id, fb->id) < 0)
		fprintf(stderr, "atomic add property failed\n");

	if (drmModeAtomicAddProperty(S->req, S->plane_id,
	 S->props.plane.in_fence_fd, in_fence_fd) < 0)
		fprintf(stderr, "atomic add property failed\n");
	iffdtc[iffdtcn++] = in_fence_fd;
}

void client_buffer_on_overlay(struct screen *S, struct fb *fb, uint32_t width,
uint32_t height, int in_fence_fd) {
	assert(S->req);

	drmModeAtomicAddProperty(S->req, S->overlay_plane_id, S->props.plane.src_x, 0 << 16); // SRC_X
	drmModeAtomicAddProperty(S->req, S->overlay_plane_id, S->props.plane.src_y, 0 << 16); // SRC_Y
	drmModeAtomicAddProperty(S->req, S->overlay_plane_id, S->props.plane.src_w, width << 16); // SRC_W
	drmModeAtomicAddProperty(S->req, S->overlay_plane_id, S->props.plane.src_h, height << 16); // SRC_H
	drmModeAtomicAddProperty(S->req, S->overlay_plane_id, S->props.plane.crtc_x, 0); // CRTC_X
	drmModeAtomicAddProperty(S->req, S->overlay_plane_id, S->props.plane.crtc_y, 0); // CRTC_Y
	drmModeAtomicAddProperty(S->req, S->overlay_plane_id, S->props.plane.crtc_w, width); // CRTC_W
	drmModeAtomicAddProperty(S->req, S->overlay_plane_id, S->props.plane.crtc_h, height); // CRTC_H
	drmModeAtomicAddProperty(S->req, S->overlay_plane_id, S->props.plane.crtc_id, S->crtc_id); // CRTC_ID

	if (drmModeAtomicAddProperty(S->req, S->overlay_plane_id, S->props.plane.fb_id, fb->id) < 0)
		fprintf(stderr, "atomic add property failed\n");

	if (drmModeAtomicAddProperty(S->req, S->plane_id,
	 S->props.plane.in_fence_fd, in_fence_fd) < 0)
		fprintf(stderr, "atomic add property failed\n");
	iffdtc[iffdtcn++] = in_fence_fd;
}

void screen_main(struct screen *S) {
	assert(S->req);

// Swap buffers TODO improve
	struct fb *tmp = S->back;
	S->back = S->front;
	S->front = tmp;

	if (drmModeAtomicAddProperty(S->req, S->plane_id,
	S->props_plane_fb_id, S->front->id) < 0)
		fprintf(stderr, "atomic add property failed\n");
}

int screen_get_gpu_fd(struct screen *S) {
	return S->gpu_fd;
}

struct box screen_get_dimensions(struct screen *S) {
	struct box box;
	box.x = 0, box.y = 0;
	drmModeFB *old_fb = drmModeGetFB(S->gpu_fd, S->old_fb_id);
	box.width = old_fb->width, box.height = old_fb->height;
	drmModeFreeFB(old_fb);
	return box;
}

struct gbm_device *screen_get_gbm_device(struct screen *screen) {
	return bufmgr_get_gbm_device(screen->bufmgr);
}

struct buffer *screen_get_back_buffer(struct screen *screen) {
	return screen->back->buffer;
}

struct buffer *screen_get_front_buffer(struct screen *screen) {
	return screen->front->buffer;
}

struct buffer *screen_get_cursor_buffer(struct screen *screen) {
	return screen->cursor->buffer;
}

bool screen_is_overlay_supported(struct screen *S) {
	return S->overlay_plane_id > 0;
}

/*
 * DRM Framebuffer manager: private code
 */
struct fb *screen_fb_create(struct screen *screen, struct buffer *buffer, bool
no_alpha) {
	uint32_t width = buffer_get_width(buffer);
	uint32_t height = buffer_get_height(buffer);
	uint32_t format = buffer_get_format(buffer);

	uint32_t handles[4] = {0};
	uint32_t strides[4] = {0};
	uint32_t offsets[4] = {0};
	uint64_t modifier[4] = {0};
	for (size_t i=0; i < buffer_get_plane_count(buffer); i++) {
		handles[i] = buffer_get_handle(buffer, i);
		strides[i] = buffer_get_stride(buffer, i);
		offsets[i] = buffer_get_offset(buffer, i);
		modifier[i] = buffer_get_modifier(buffer);
	}

	if (no_alpha)
	// XRGB8888 is more likely to be supported
	format = format == DRM_FORMAT_ARGB8888 ? DRM_FORMAT_XRGB8888 : format;

	uint32_t buf_id;
	if (drmModeAddFB2WithModifiers(screen->gpu_fd, width, height, format,
	 handles, strides, offsets, modifier, &buf_id, DRM_MODE_FB_MODIFIERS)) {
		perror("drmModeAddFB2WithModifiers");
		abort();
		return NULL;
	}

	struct fb *fb = malloc(sizeof(struct fb));
	fb->id = buf_id;
	fb->buffer = buffer;
	return fb;
}

void screen_fb_destroy(struct screen *screen, struct fb *fb) {
	buffer_destroy(fb->buffer);
	drmModeRmFB(screen->gpu_fd, fb->id);
	free(fb);
}

/*
 * DRM Framebuffer manager: public code
 */
struct fb *screen_fb_create_main(struct screen *screen, int width, int height,
bool linear, bool no_alpha) {
	struct buffer *buffer = bufmgr_buffer_create(screen->bufmgr, width,
	 height, linear, !no_alpha);
	if (!buffer)
		return NULL;
	return screen_fb_create(screen, buffer, no_alpha);
}

struct fb *screen_fb_create_from_dmabuf(struct screen *screen, int32_t width,
int32_t height, uint32_t format, uint32_t num_planes, int32_t *fds, uint32_t
*offsets, uint32_t *strides, uint64_t *modifiers) {
	struct buffer *buffer = bufmgr_buffer_import_from_dmabuf(screen->bufmgr,
	 num_planes, fds, width, height, format, strides, offsets, modifiers[0]);
	return screen_fb_create(screen, buffer, true);
}

void screen_fb_schedule_destroy(struct screen *screen, struct fb *fb) {
	struct fb_node *fb_node = malloc(sizeof(struct fb_node));
	fb_node->fb = fb;
	wl_list_insert(&screen->fb_destroy_list, &fb_node->link);
}

void screen_release(struct screen *S) {
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	if (drmModeAtomicAddProperty(req, S->plane_id, S->props_plane_fb_id, S->old_fb_id) < 0)
		fprintf(stderr, "atomic add property failed\n");
	// TODO: unbind overlay planes
	if(drmModeAtomicCommit(S->gpu_fd, req, 0, 0))
		perror("atomic commit failed");
	
	drmModeAtomicFree(req);

	/*
	 * Client buffers clean-up
	 */
	struct fb_node *fb_node, *tmp;
	wl_list_for_each_safe(fb_node, tmp, &S->fb_destroy_list, link) {
		screen_fb_destroy(S, fb_node->fb);
		wl_list_remove(&fb_node->link);
		free(fb_node);
	}

	screen_fb_destroy(S, S->back);
	screen_fb_destroy(S, S->front);
	bufmgr_destroy(S->bufmgr);
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

uint32_t get_active_fb(int fd, uint32_t plane_id) {
	drmModePlane *plane = drmModeGetPlane(fd, plane_id);
	uint32_t fb_id = plane->fb_id;
	drmModeFreePlane(plane);
	return fb_id;
}

const char *screen_get_bufmgr_impl(struct screen *screen) {
	return bufmgr_get_name(screen->bufmgr);
}

const char *conn_get_name(uint32_t type_id) {
	switch (type_id) {
	case DRM_MODE_CONNECTOR_Unknown:     return "Unknown";
	case DRM_MODE_CONNECTOR_VGA:         return "VGA";
	case DRM_MODE_CONNECTOR_DVII:        return "DVI-I";
	case DRM_MODE_CONNECTOR_DVID:        return "DVI-D";
	case DRM_MODE_CONNECTOR_DVIA:        return "DVI-A";
	case DRM_MODE_CONNECTOR_Composite:   return "Composite";
	case DRM_MODE_CONNECTOR_SVIDEO:      return "SVIDEO";
	case DRM_MODE_CONNECTOR_LVDS:        return "LVDS";
	case DRM_MODE_CONNECTOR_Component:   return "Component";
	case DRM_MODE_CONNECTOR_9PinDIN:     return "DIN";
	case DRM_MODE_CONNECTOR_DisplayPort: return "DP";
	case DRM_MODE_CONNECTOR_HDMIA:       return "HDMI-A";
	case DRM_MODE_CONNECTOR_HDMIB:       return "HDMI-B";
	case DRM_MODE_CONNECTOR_TV:          return "TV";
	case DRM_MODE_CONNECTOR_eDP:         return "eDP";
	case DRM_MODE_CONNECTOR_VIRTUAL:     return "Virtual";
	case DRM_MODE_CONNECTOR_DSI:         return "DSI";
#ifdef DRM_MODE_CONNECTOR_DPI
	case DRM_MODE_CONNECTOR_DPI:         return "DPI";
#endif
	default:                             return "Unknown";
	}
}
