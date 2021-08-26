#ifndef gbm_h_INCLUDED
#define gbm_h_INCLUDED

void gbm_init(int drm_fd);
struct gbm_device *gbm_get_device();
void gbm_fini();

#endif // gbm_h_INCLUDED

