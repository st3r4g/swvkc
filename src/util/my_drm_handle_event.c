#include <xf86drm.h>

#include <stdbool.h>
#include <unistd.h>

#define U642VOID(x) ((void *)(unsigned long)(x))

/*
 * Function modified so that page_flip_handler gets called before vblank_handler
 */

int my_drmHandleEvent(int fd, drmEventContextPtr evctx)
{
	char buffer[1024];
	int len, i;
	struct drm_event *e;
	struct drm_event_vblank *vblank;
	struct drm_event_crtc_sequence *seq;
	void *user_data;

	bool vblank_handler_call = false;
	bool page_flip_handler_call = false;

	/* The DRM read semantics guarantees that we always get only
	 * complete events. */

	len = read(fd, buffer, sizeof buffer);
	if (len == 0)
		return 0;
	if (len < (int)sizeof *e)
		return -1;

	i = 0;
	while (i < len) {
		e = (struct drm_event *)(buffer + i);
		switch (e->type) {
		case DRM_EVENT_VBLANK:
			if (evctx->version < 1 ||
			    evctx->vblank_handler == NULL)
				break;
			vblank = (struct drm_event_vblank *) e;
			vblank_handler_call = true;
/*			evctx->vblank_handler(fd,
					      vblank->sequence,
					      vblank->tv_sec,
					      vblank->tv_usec,
					      U642VOID (vblank->user_data));*/
			break;
		case DRM_EVENT_FLIP_COMPLETE:
			vblank = (struct drm_event_vblank *) e;
			user_data = U642VOID (vblank->user_data);

			if (evctx->version >= 3 && evctx->page_flip_handler2)
				evctx->page_flip_handler2(fd,
							 vblank->sequence,
							 vblank->tv_sec,
							 vblank->tv_usec,
							 vblank->crtc_id,
							 user_data);
			else if (evctx->version >= 2 && evctx->page_flip_handler)
				page_flip_handler_call = true;
/*				evctx->page_flip_handler(fd,
							 vblank->sequence,
							 vblank->tv_sec,
							 vblank->tv_usec,
							 user_data);*/
			break;
		case DRM_EVENT_CRTC_SEQUENCE:
			seq = (struct drm_event_crtc_sequence *) e;
			if (evctx->version >= 4 && evctx->sequence_handler)
				evctx->sequence_handler(fd,
							seq->sequence,
							seq->time_ns,
							seq->user_data);
			break;
		default:
			break;
		}
		i += e->length;
	}

	if (page_flip_handler_call) {
		evctx->page_flip_handler(fd,
					 vblank->sequence,
					 vblank->tv_sec,
					 vblank->tv_usec,
					 user_data);
	}
	
	if (vblank_handler_call) {
		evctx->vblank_handler(fd,
				      vblank->sequence,
				      vblank->tv_sec,
				      vblank->tv_usec,
				      U642VOID (vblank->user_data));
	}

	return 0;
}
