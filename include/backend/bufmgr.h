#include <inttypes.h>
#include <stdbool.h>

struct bufmgr *bufmgr_create(int drm_fd);
void bufmgr_destroy(struct bufmgr *self);
const char *bufmgr_get_name(struct bufmgr *self);

struct buffer *bufmgr_buffer_create(struct bufmgr *self, int width, int height,
bool linear, int format);
//struct buffer *bufmgr_buffer_import_from_dmabuf(struct bufmgr *self, int
//num_fds, int32_t *dmabuf_fds);
struct buffer *bufmgr_buffer_import_from_dmabuf(struct bufmgr *self, int
num_fds, int32_t *dmabuf_fds,
uint32_t width, uint32_t height, uint32_t format, uint32_t *strides, uint32_t *offsets,
uint64_t modifier);
void buffer_destroy(struct buffer *self);

uint32_t buffer_get_width(struct buffer *self);
uint32_t buffer_get_height(struct buffer *self);
uint32_t buffer_get_format(struct buffer *self);
uint32_t buffer_get_plane_count(struct buffer *self);
uint32_t buffer_get_handle(struct buffer *self, int i);
uint32_t buffer_get_stride(struct buffer *self, int i);
uint32_t buffer_get_offset(struct buffer *self, int i);
uint64_t buffer_get_modifier(struct buffer *self);
int buffer_get_fd(struct buffer *self);

/* GBM specific stuff */

struct gbm_device *bufmgr_get_gbm_device(struct bufmgr *self);
