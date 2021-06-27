int vulkan_init(bool *dmabuf, bool *dmabuf_mod, int64_t *major, int64_t *minor);
void vulkan_render_shm_buffer(uint32_t width, uint32_t height, uint32_t stride,
uint32_t format, uint8_t *data);
int vulkan_main(int i, int fd, int width, int height, int stride, uint64_t modifier);
void vulkan_create_screen_image(struct buffer *back, struct buffer *front);
void vulkan_create_cursor_image(struct buffer *buffer);
void vulkan_render_shm_buffer_cursor(uint32_t width, uint32_t height, uint32_t stride,
uint32_t format, uint8_t *data);
