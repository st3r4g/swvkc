int vulkan_init(int fd);
void vulkan_render_shm_buffer(uint32_t width, uint32_t height, uint32_t stride,
uint32_t format, uint8_t *data);
int vulkan_main(int i, int fd, int width, int height, int stride);
