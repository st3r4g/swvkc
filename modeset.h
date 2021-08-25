#ifndef modeset_h_INCLUDED
#define modeset_h_INCLUDED

#include <stdint.h>

void modeset_init();
void modeset_draw();
void modeset_copy(uint32_t stride, uint32_t width, uint32_t height, uint8_t* m);
void modeset_cleanup();

#endif // modeset_h_INCLUDED

