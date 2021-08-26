#ifndef gbm_h_INCLUDED
#define gbm_h_INCLUDED

void gbm_init(int drm_fd);
struct gbm_device *gbm_get_device();
uint32_t gbm_import_from_dmabuf(int num_fds, int32_t *dmabuf_fds, uint32_t
width, uint32_t height, uint32_t format, uint32_t *strides, uint32_t *offsets,
uint64_t modifier);
void gbm_fini();

#endif // gbm_h_INCLUDED

