#include <gbm.h>

#include <assert.h>

static struct gbm_device *gbm;

void gbm_init(int drm_fd) {
	gbm = gbm_create_device(drm_fd);
	assert(gbm);
}

struct gbm_device *gbm_get_device() {
	return gbm;
}

void gbm_fini() {
	gbm_device_destroy(gbm);
}
