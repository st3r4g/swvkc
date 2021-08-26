#ifndef atomic_h_INCLUDED
#define atomic_h_INCLUDED

#include <stdint.h>

int atomic_commit(uint32_t width, uint32_t height, uint32_t format, uint32_t handle, uint32_t pitch, uint32_t offset, uint64_t modifier);
void drm_read();

#endif // atomic_h_INCLUDED

