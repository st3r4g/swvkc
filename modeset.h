#ifndef modeset_h_INCLUDED
#define modeset_h_INCLUDED

#include <stdint.h>

void modeset_init();
int modeset_get_fd();
uint32_t modeset_get_crtc();
uint32_t modeset_get_conn();
uint32_t modeset_get_fb();
uint32_t modeset_get_width();
uint32_t modeset_get_height();
void modeset_draw();
void modeset_copy(uint32_t stride, uint32_t width, uint32_t height, uint8_t* m);
void modeset_cleanup();

#endif // modeset_h_INCLUDED

