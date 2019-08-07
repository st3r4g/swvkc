#include <util/log.h>

#include <drm_fourcc.h>

#include <intel_bufmgr.h>
#include <i915_drm.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Implementation of the buffer manager for Intel GPUs
 */

struct bufmgr {
	drm_intel_bufmgr *bufmgr;
};

struct buffer {
	uint32_t width;
	uint32_t height;
	uint32_t format;
	int bo_count;
	drm_intel_bo **bos;
	uint32_t *strides;
	uint32_t *offsets;
	uint64_t modifier;
};

struct bufmgr *bufmgr_create(int drm_fd) {
	struct bufmgr *self = malloc(sizeof(struct bufmgr));
/*
 * What is batch_size? Is 32 an acceptable value?
 */
	self->bufmgr = drm_intel_bufmgr_gem_init(drm_fd, 32);
	if (!self->bufmgr) {
		fprintf(stderr, "drm_intel_bufmgr_gem_init failed\n");
		return NULL;
	}
	return self;
}

void bufmgr_destroy(struct bufmgr *self) {
	drm_intel_bufmgr_destroy(self->bufmgr);
	free(self);
}

struct buffer *bufmgr_buffer_create(struct bufmgr *self, int width, int height) {
	int bpp = 32;
	uint32_t tiling = I915_TILING_NONE;
	long unsigned int stride;
	drm_intel_bo *bo = drm_intel_bo_alloc_tiled(self->bufmgr, "main", width,
	 height, bpp/8, &tiling, &stride, 0);
	if (!bo) {
		fprintf(stderr, "drm_intel_bo_alloc failed\n");
		return NULL;
	}
	printf("intel_bufmgr: allocated %dx%d, stride: %lu\n", width, height,
	 stride);
	
	struct buffer *buffer = malloc(sizeof(struct buffer));
	buffer->width = width;
	buffer->height = height;
/*
 * I'll guess the format here...
 */
	buffer->format = DRM_FORMAT_XRGB8888;
	buffer->bo_count = 1;
	buffer->bos = malloc(sizeof(struct drm_intel_bo *));
	buffer->bos[0] = bo;
	buffer->strides = malloc(sizeof(uint32_t));
	buffer->strides[0] = stride;
	buffer->offsets = malloc(sizeof(uint32_t));
/*
 * I'll guess the offset here...
 */
	buffer->offsets[0] = 0;
	switch (tiling) {
		case I915_TILING_NONE:
		buffer->modifier = DRM_FORMAT_MOD_LINEAR;
		break;
		case I915_TILING_X:
		buffer->modifier = I915_FORMAT_MOD_X_TILED;
		break;
		case I915_TILING_Y:
		buffer->modifier = I915_FORMAT_MOD_Y_TILED;
		break;
		default:
		buffer->modifier = DRM_FORMAT_MOD_INVALID;
	}
	return buffer;
}

struct buffer *bufmgr_buffer_import_from_dmabuf(struct bufmgr *self, int
num_fds, int32_t *dmabuf_fds,
uint32_t width, uint32_t height, uint32_t format, uint32_t *strides, uint32_t *offsets,
uint64_t modifier) {
	drm_intel_bo **bos = malloc(num_fds*sizeof(struct drm_intel_bo *));
	for (int i=0; i<num_fds; i++) {
		bos[i] = drm_intel_bo_gem_create_from_prime(self->bufmgr,
		 dmabuf_fds[i], 0);
		if (!bos[i]) {
			fprintf(stderr, "drm_intel_bo_gem_create_from_prime failed\n");
			free(bos);
			return NULL;
		}
	}
	printf("good import\n");
	struct buffer *buffer = malloc(sizeof(struct buffer));
	buffer->width = width;
	buffer->height = height;
	buffer->format = format;
	buffer->bo_count = num_fds;
	buffer->bos = bos;
/*
 * The wl_buffer_dmabuf memory should stay valid
 */
	buffer->strides = strides;
	buffer->offsets = offsets;
	buffer->modifier = modifier;
	return buffer;
}

void buffer_destroy(struct buffer *self) {
	for (int i=0; i<self->bo_count; i++)
		drm_intel_bo_unreference(self->bos[i]);
	free(self->bos);
	free(self);
}

uint32_t buffer_get_width(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_width: invalid buffer\n");
		return 0;
	}
	return self->width;
}

uint32_t buffer_get_height(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_height: invalid buffer\n");
		return 0;
	}
	return self->height;
}

uint32_t buffer_get_format(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_format: invalid buffer\n");
		return 0;
	}
	return self->format;
}

uint32_t buffer_get_plane_count(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_plane_count: invalid buffer\n");
		return 0;
	}
	return self->bo_count;
}

uint32_t buffer_get_handle(struct buffer *self, int i) {
	if (!self) {
		fprintf(stderr, "buffer_get_handle: self is NULL\n");
		return 0;
	} else if (!self->bos[i]) {
		fprintf(stderr, "buffer_get_handle: buffer is invalid\n");
		return 0;
	}
	return self->bos[i]->handle;
}

uint32_t buffer_get_stride(struct buffer *self, int i) {
	if (!self) {
		fprintf(stderr, "buffer_get_stride: invalid buffer\n");
		return 0;
	}
	return self->strides[i];
}

uint32_t buffer_get_offset(struct buffer *self, int i) {
	if (!self) {
		fprintf(stderr, "buffer_get_offset: invalid buffer\n");
		return 0;
	}
	return self->offsets[i];
}

uint64_t buffer_get_modifier(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_modifier: invalid buffer\n");
		return 0;
	}
	return self->modifier;
}

int buffer_get_fd(struct buffer *self) {
	if (!self) {
		fprintf(stderr, "buffer_get_fd: invalid buffer\n");
		return -1;
	}
	int prime_fd;
	drm_intel_bo_gem_export_to_prime(self->bos[0], &prime_fd);
	return prime_fd;
}

/* GBM specific stuff */

struct gbm_device *bufmgr_get_gbm_device(struct bufmgr *self) {
	return NULL;
}
