#ifndef MYSCREEN_H
#define MYSCREEN_H

#include <stdbool.h>
#include <util/box.h>

struct screen;

/*
 * The screen interface represents a physical screen where the rendering done by
 * the Wayland compositor is presented.
 */

struct screen *screen_setup(void (*vblank_notify)(int,unsigned int,unsigned int,
unsigned int, void*, bool), void *user_data, void (*listen_to_out_fence)(int,
void*), void *user_data2, bool dmabuf_mod);

void drm_handle_event(int fd);

int screen_get_gpu_fd(struct screen *);
int screen_get_bo_fd(struct screen *);
uint32_t screen_get_bo_stride(struct screen *);
uint64_t screen_get_bo_modifier(struct screen *S);
struct gbm_device *screen_get_gbm_device(struct screen *);
struct box screen_get_dimensions(struct screen *S);
bool screen_is_overlay_supported(struct screen *S);
void screen_post_direct(struct screen *, uint32_t width, uint32_t height,
uint32_t format, int fd, int stride, int offset, uint64_t modifier);
void screen_post(struct screen *S, int fence_fd);

void screen_release(struct screen *);

#endif
