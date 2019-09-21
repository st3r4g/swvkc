#include <backend/bufmgr.h>

#include <gbm.h>

#include <stdio.h>
#include <stdlib.h>

/*
 * Implementation of the buffer manager via GBM
 *
 * ISSUES:
 * weston-simple-dmabuf-v4l: buffer_get_plane_count returns 2
 */

struct bufmgr {
	struct gbm_device *gbm_device;
};

struct buffer {
	struct gbm_bo *gbm_bo;
};

struct bufmgr *bufmgr_create(int drm_fd) {
	struct gbm_device *gbm_device = gbm_create_device(drm_fd);
	if (!gbm_device) {
		fprintf(stderr, "gbm_create_device failed\n");
		return NULL;
	}

	struct bufmgr *self = malloc(sizeof(struct bufmgr));
	self->gbm_device = gbm_device;
	return self;
}

void bufmgr_destroy(struct bufmgr *self) {
	if (!self) {
		fprintf(stderr, "bufmgr_destroy: invalid bufmgr\n");
		return;
	}
	gbm_device_destroy(self->gbm_device);
	free(self);
}

const char *bufmgr_get_name(struct bufmgr *self) {
	return "GBM";
}

struct buffer *bufmgr_buffer_create(struct bufmgr *self, int width, int height,
bool linear) {
	if (!self) {
		fprintf(stderr, "bufmgr_buffer_create: invalid bufmgr\n");
		return NULL;
	}

	uint32_t flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;
	if (linear)
		flags |= GBM_BO_USE_LINEAR;

	if (!gbm_device_is_format_supported(self->gbm_device,
	 GBM_BO_FORMAT_XRGB8888, flags)) {
		fprintf(stderr, "format unsupported\n");
		return NULL;
	}

	struct gbm_bo *gbm_bo = gbm_bo_create(self->gbm_device, width,
	 height, GBM_BO_FORMAT_XRGB8888, flags);

/*	uint64_t modifier = I915_FORMAT_MOD_X_TILED;
	S->gbm_bo = gbm_bo_create_with_modifiers(S->gbm_device,
			screen_size.width, screen_size.height,
			GBM_BO_FORMAT_XRGB8888, &modifier, 1);*/
	if (!gbm_bo) {
		fprintf(stderr, "gbm_bo_create failed\n");
		return NULL;
	}

	struct buffer *buffer = malloc(sizeof(struct buffer));
	buffer->gbm_bo = gbm_bo;
	return buffer;
}

struct buffer *bufmgr_buffer_import_from_dmabuf(struct bufmgr *self, int
num_fds, int32_t *dmabuf_fds,
uint32_t width, uint32_t height, uint32_t format, uint32_t *strides, uint32_t *offsets,
uint64_t modifier) {
	if (!self) {
		fprintf(stderr, "bufmgr_buffer_import_from_dmabuf: invalid bufmgr\n");
		return NULL;
	}
	struct gbm_import_fd_modifier_data data = {
		.width = width,
		.height = height,
		.format = format,
		.num_fds = 1,
		.fds[0] = dmabuf_fds[0],
		.strides[0] = strides[0],
		.offsets[0] = offsets[0],
		.modifier = modifier
	};
	struct gbm_bo *gbm_bo = gbm_bo_import(self->gbm_device,
	 GBM_BO_IMPORT_FD_MODIFIER, &data, GBM_BO_USE_SCANOUT);

	if (!gbm_bo) {
		fprintf(stderr, "gbm_bo_import failed\n");
		return NULL;
	}

	struct buffer *buffer = malloc(sizeof(struct buffer));
	buffer->gbm_bo = gbm_bo;
	return buffer;
}

void buffer_destroy(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_destroy: invalid buffer\n");
		return;
	}
	gbm_bo_destroy(self->gbm_bo);
	free(self);
}

uint32_t buffer_get_width(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_width: invalid buffer\n");
		return 0;
	}
	return gbm_bo_get_width(self->gbm_bo);
}

uint32_t buffer_get_height(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_height: invalid buffer\n");
		return 0;
	}
	return gbm_bo_get_height(self->gbm_bo);
}

uint32_t buffer_get_format(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_format: invalid buffer\n");
		return 0;
	}
	return gbm_bo_get_format(self->gbm_bo);
}

uint32_t buffer_get_plane_count(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_plane_count: invalid buffer\n");
		return 0;
	}
	return gbm_bo_get_plane_count(self->gbm_bo);
}

uint32_t buffer_get_handle(struct buffer *self, int i) {
	if (!self) {
		fprintf(stderr, "buffer_get_handle: invalid buffer\n");
		return 0;
	}
	return gbm_bo_get_handle_for_plane(self->gbm_bo, i).u32;
}

uint32_t buffer_get_stride(struct buffer *self, int i) {
	if (!self) {
		fprintf(stderr, "buffer_get_stride: invalid buffer\n");
		return 0;
	}
	return gbm_bo_get_stride_for_plane(self->gbm_bo, i);
}

uint32_t buffer_get_offset(struct buffer *self, int i) {
	if (!self) {
		fprintf(stderr, "buffer_get_offset: invalid buffer\n");
		return 0;
	}
	return gbm_bo_get_offset(self->gbm_bo, i);
}

uint64_t buffer_get_modifier(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_modifier: invalid buffer\n");
		return 0;
	}
	return gbm_bo_get_modifier(self->gbm_bo);
}

int buffer_get_fd(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_fd: invalid buffer\n");
		return -1;
	}
	return gbm_bo_get_fd(self->gbm_bo);
}

/* GBM specific stuff */

struct gbm_device *bufmgr_get_gbm_device(struct bufmgr *self) {
	if (!self) {
		fprintf(stderr, "bufmgr_get_gbm_device: invalid bufmgr\n");
		return 0;
	}
	return self->gbm_device;
}
